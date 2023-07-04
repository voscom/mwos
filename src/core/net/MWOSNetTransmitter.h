#ifndef MWOS3_MWOSNETTRANSMITTER_H
#define MWOS3_MWOSNETTRANSMITTER_H
/***
 * Базовый модуль сетевой передачи
 *
    Запись поточно (без буффера) от контроллера серверу:
    4b: {02"MW"03} - признак начала пакета.
    1b: {команда серверу} MWOSProtocolCommand (MWOSConsts.h)
    2b: {id модуля} - начинаем считать контрольную сумму (включая id модуля)
    2b: {id параметра}
    данные параметра, вообще нихрена не ограниченное количество
    4b: {03"MW"03} - признак конца пакета (входит в контрольную сумму)
    2b: {контрольная сумма блока}
 *
 * При записи - длина блока не передается, а конец блока сообщается по фрейму sendEndBlock {03"MW"03}
 *
 */

#include <core/adlib/MWTimeout.h>
#include <core/adlib/LibCRC.h>
#include <core/adlib/MWArduinoLib.h>
#include "core/MWOSModule.h"
#include "core/MWOSParam.h"

#ifndef MWOS_SEND_BUFFER_SIZE
// размер кольцевого буффера для отправляемых данных
#define MWOS_SEND_BUFFER_SIZE 1024
#endif

#ifndef MWOS_RECIVE_TIMEOUT_DSEC
#define MWOS_RECIVE_TIMEOUT_DSEC 300 // (сек/10) таймаут прерывания связи, если ничего не приходило больше этого времени. Обычно, это время необходимо и серверу, что-бы определить потерю связи
#endif

#ifndef MWOS_CONTROLLER_TYPE
#define MWOS_CONTROLLER_TYPE 0 // тип контроллера по умолчанию
#endif

#ifndef MWOS_USER_HASH
#define MWOS_USER_HASH 0
#endif

extern MWOS3 mwos;

class MWOSNetTransmitter : Print {
public:

#pragma pack(push,1)
    // для отправки данных
    bool needSendFormat=false;
    bool IsConnected=false; // подключено к сереру
    MWOSModuleBase * sendModule=NULL;
    MWOSParam * sendParam=NULL;
    uint16_t sendParamIndex=0;
#ifdef MWOS_SEND_BUFFER_USE
    uint8_t sendBuffer[MWOS_SEND_BUFFER_SIZE]; // кольцевой буффер отправляемых данных
    uint16_t writeOffset; // текущая позиция для записи в буффер
    uint16_t sendOffset; // текущая позиция в буффере для отправки на сервер
#endif
    MW_CRC16 send_crc16;
    Stream * _stream;
#pragma pack(pop)

    void sendUpdate() {
#ifdef MWOS_SEND_BUFFER_USE
        if (writeOffset != sendOffset) { // отправим данные из буффера отправки
            while (writeOffset != sendOffset && _stream->availableForWrite() > 0) {
                if (_stream->write(sendBuffer[sendOffset])==0) {
                    return; // никак не отправляется
                }
                sendOffset++; if (sendOffset>=MWOS_SEND_BUFFER_SIZE) sendOffset=0;
            }
            return;
        }
        writeOffset=0;
        sendOffset=0;
#endif
        if (needSendFormat) writeFormatController();
        else if (IsConnected) writeChangedModules();
    }

    /***
     * Отправить на сервер хендшейк (не проверяет переполнение буффера, потому что отправляется в начале сессии)
     * @return  Размер имени проекта + 40 байт
     */
    uint16_t writeHandshake(uint32_t cid) {
        if (this->availableForWrite()<44) {
            //write(0);
            return 0;
        }
        MW_LOG(F("writeHandshake: ")); MW_LOG(cid); MW_LOG(':'); MW_LOG_LN(this->availableForWrite());
        uint16_t size=sendBeginBlockParam(mwos_server_cmd_handshake); // 5
        size+=writeUInt32(cid); // 7 код контроллера
        size+=writeUInt64(MWOS_USER_HASH); // 15 отправим MW_User_Hash
        size+=writeUInt32(getChipID()); // 19 отправим chipID
        size+=write_progmem_str((char *) mwos_project); // имя проекта
        size+=write(0); // 20 конец строки
        size+=write_progmem_str((char *) git_hash_proj,8); // 28 версия проекта
        size+=write('_'); // 29
        size+=write_progmem_str((char *) git_hash_mwos,8); // 37 версия mwos
        size+=write(0); // 39 конец строки
        size+=write(mwos.storagesMask); // 41 состояния хранилищ
        size+=sendEndBlock();
        return size;
    }

    uint16_t writeUInt32(uint32_t v) {
        write((uint8_t *) &v,4);
        return 4;
    }

    uint16_t writeInt32(int32_t v) {
        write((uint8_t *) &v,4);
        return 4;
    }

    uint16_t writeInt64(int64_t v) {
        write((uint8_t *) &v,8);
        return 8;
    }

    uint16_t writeUInt64(uint64_t v) {
        write((uint8_t *) &v,8);
        return 8;
    }

    /***
     * Команда, начинающая отправку формата контроллера на сервер
     */
    void sendFormatController() {
        MW_LOG(F("sendFormatController: ")); MW_LOG_LN(this->availableForWrite());
        sendModule=NULL;
        sendParam=NULL;
        needSendFormat=true;
    }

    /***
     * Если задано needSendFormat отправляет формат контроллера на сервер
     * следит за переполнением буффера отправки и отправляет продолжение, когда место появится
     */
    void writeFormatController() {
        if (sendModule==NULL && sendParam==NULL) {
            int16_t controllerInfoSize= mwos.nameSize()+150;
            if (availableForWrite()<=controllerInfoSize) { // нет места для отправки начала блока и описания контроллера
                return;
            }
            sendBeginBlockParam(mwos_server_cmd_get_format); // 8 признак, что дальше идет формат контроллера
            writeUInt16(mwos.modulesCount); // 10 количество модулей
            write(mwos.storageCount); // 10 количество хранилищ
            write(MWOS_CONTROLLER_TYPE); // 11 тип контроллера
            write(getPlatform()); // 12 тип платформы микроконтроллера
            writeUInt16((uint16_t) MWOS_SEND_BUFFER_SIZE); // 14 максимальный размер буффера отправки
            writeUInt16((uint16_t) MWOS_RECIVE_TIMEOUT_DSEC); // 16 таймаут приема
            write_progmem_str((char *) git_hash_proj); // +40 = 56 GIT хеш проекта (до 40 символов)
            write(0); // 57 конец строки
            write_progmem_str((char *) git_hash_mwos); // +40 = 96 GIT хеш mwos (до 40 символов)
            write(0); // 97 конец строки
            mwos.printName(this); // описание контроллера
            write(0); // 98 конец строки
            write_progmem_str((char *) mwos_board,40); // 99 BOARD (до 40 байт)
            write(0); // 141 конец строки
            sendModule=(MWOSModuleBase *) mwos.child;
            sendParamIndex=0;
        }
        while (sendModule!=NULL) {
            if (sendParam==NULL) { // сначала отправим данные модуля
                // {имя} + {uint8 #0} + {uint16 id} + {uint16 count}
                int16_t moduleFormatSize= sendModule->nameSize()+6;
                if (availableForWrite()<=moduleFormatSize) { // нет места для отправки модуля
                    return;
                }
                sendModule->printName(this); // имя
                write(0); // 1 конец строки
                writeUInt16(sendModule->id); // 3 id модуля
                write(sendModule->moduleType); // 4 тип модуля
                writeUInt16(sendModule->paramsCount); //  6 количество параметров
                sendParam=(MWOSParam *) sendModule->child;
            }
            // отправим данные параметров
            while (sendParam!=NULL) {
                int16_t paramFormatSize=sendParam->nameSize()+9;
                if (availableForWrite()<=paramFormatSize) { // нет места для отправки параметра
                    return;
                }
                writeUInt16(sendParam->id); // 2 id параметра (может и не соответствовать порядковому номеру)
                writeInt16(sendParam->valueType); // 6 тип значения параметра
                write(sendParam->group); // 7 группа параметра
                write((uint8_t) sendParam->storage); // 8 место хранения
                sendParam->printName(this); // имя
                write(0); // 9 конец строки
                //MW_LOG(F("SendParamFormat: ")); MW_LOG(sendModule->id); MW_LOG(':'); MW_LOG_LN(sendParam->id);
                sendParam=(MWOSParam *) sendParam->next;
                if (sendParam!=NULL && sendParam->unitType!=UnitType::PARAM) sendParam=NULL;

            }
            sendParam=NULL;
            sendModule=(MWOSModuleBase *) sendModule->next;
            if (sendModule!=NULL && sendModule->unitType!=UnitType::MODULE) sendModule=NULL;
        }
        if (availableForWrite()<=6) { // нет места для отправки концевика блока
            return;
        }
        sendEndBlock();
        ToSendModule(mwos.child);
        needSendFormat=false;
    }

    /**
     * Отправляет все измененные параметры на сервер отдельными блоками
     * следит за переполнением буффера отправки и отправляет продолжение, когда место появится
     */
    void writeChangedModules() {
        while (sendModule!=NULL) {
            if (sendModule->changedMask.haveSetBits()) { // только, если есть установленные биты изменения параметров для модуля
                while (sendParam!=NULL) {
                    while (sendParamIndex < sendParam->arrayCount()) {
                        if (sendModule->IsParamChanged(sendParam, sendParamIndex) && !sendParam->IsGroup(mwos_param_secret)) {
                            if ((int16_t) sendParam->byteSize(false) + 14 < availableForWrite()) {
                                // отправим значение
                                MW_LOG(F("sendValues: ")); MW_LOG(sendModule->id); MW_LOG(':'); MW_LOG(sendParam->id); MW_LOG(':'); MW_LOG_LN(sendParamIndex);
                                sendBeginBlockParam(mwos_server_cmd_param_set_value,sendModule->id,sendParam->id,sendParamIndex); // 11
                                uint16_t byteSize=sendParam->byteSize(false);
                                int64_t value= sendModule->getValue(sendParam, sendParamIndex);
                                uint8_t * block=(uint8_t *) &value;
                                for (uint16_t i = 0; i < byteSize; ++i) {
                                    write(block[i]);
                                }
                                sendEndBlock(); // 14
                            } else return; // нет места для отправки блока
                        }
                        sendModule->SetParamChanged(sendParam, sendParamIndex, false); // сбросим бит отправки
                        sendParamIndex++;
                    }
                    ToSendParam(sendParam->next); // следующий параметр
                }
            }
            ToSendModule(sendModule->next); // следующий модуль
        }
        ToSendModule(mwos.child); // к первому модулю
    }

    void ToSendModule(MWOSUnit * toUnit) {
        sendModule=(MWOSModuleBase *) toUnit;
        if (sendModule!=NULL && sendModule->unitType!=UnitType::MODULE) sendModule=NULL;
        if (sendModule!=NULL) ToSendParam(sendModule->child);
        else ToSendParam(NULL);
    }

    void ToSendParam(MWOSUnit * toUnit) {
        sendParam=(MWOSParam *) toUnit;
        if (sendParam!=NULL && sendParam->unitType!=UnitType::PARAM) sendParam=NULL;
        sendParamIndex=0;
    }


    /***
     * Отправить признак начала блока с id-модуля и id-параметра (8 байт)
     * @param moduleId
     * @param paramId
     * @return
     */
    uint16_t sendBeginBlockParam(MWOSProtocolCommand cmd, uint16_t moduleId=0, uint16_t paramId=0, uint16_t arrayIndex=0) {
        write(02); // 1 признак начала пакета
        writePacketSign(); // 4 подпись пакета
        send_crc16.start();
        write(cmd); // 5
        if (cmd>127) return 5;
        writeUInt16(moduleId); // 7
        if (cmd>63) return 7;
        writeUInt16(paramId); // 9
        if (cmd>31) return 9;
        writeUInt16(arrayIndex); // 11
        return 11;
    }

    /***
     * Отправить концевик блока с контрольной суммой (6 байт)
     * @return
     */
    uint16_t sendEndBlock() {
        write(03); // 1 признак конца пакета
        writePacketSign(); // 4 подпись пакета
        // отправим контрольную сумму
        writeUInt16(send_crc16.crc); // 6
        return 6;
    }

    /**
     * Отправить духбайтовое целое в порт контроллера
     */
    uint16_t writeInt16(int16_t n) {
        write((uint8_t) (n & 0xff));
        write((uint8_t) ((n >> 8) & 0xff));
        return 2;
    }

    /**
     * Отправить духбайтовое целое без знака в порт контроллера
     */
    uint16_t writeUInt16(uint16_t n) {
        write((uint8_t) (n & 0xff));
        write((uint8_t) ((n >> 8) & 0xff));
        return 2;
    }

    /**
     * Отправить метку начала пакета (или конца пакета) и размер пакета в порт контроллера
     * @param endPacket    true - признак конца пакет, false - признак начала пакета
     * @return
     */
    uint16_t writePacketSign() {
        write('M'); // признак пакета
        write('W'); // признак пакета
        write(03); // признак пакета
        return 3;
    }

    virtual size_t write(uint8_t byte) {
        send_crc16.add(byte);
#ifdef MWOS_SEND_BUFFER_USE
        sendBuffer[writeOffset]=byte;
        writeOffset++; if (writeOffset >= MWOS_SEND_BUFFER_SIZE) writeOffset=0;
#else
        if (_stream!=NULL) return _stream->write(byte);
#endif
        return 1;
    }

    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        if ((int) size>availableForWrite()) return 0;
        while(size--) {
            n += write(*buffer);
            buffer++;
        }
        return n;
    }

    virtual int availableForWrite() {
#ifdef MWOS_SEND_BUFFER_USE
        if (writeOffset>sendOffset) return MWOS_SEND_BUFFER_SIZE - (writeOffset-sendOffset);
        if (writeOffset<sendOffset) return sendOffset - writeOffset;
        return MWOS_SEND_BUFFER_SIZE;
#else
        if (_stream!=NULL) return _stream->availableForWrite();
        return 0;
#endif
    }

    /***
     * Напечатать строку из PROGMEM
     * @param pmem_str
     * @return
     */
    size_t write_progmem_str( char * progmem_str, size_t maxSize=0) {
        size_t size=strlen_P(progmem_str);
        if (maxSize>0 && maxSize<size) size=maxSize;
        for (size_t i = 0; i < size; ++i) {
            char ch = pgm_read_byte_near(progmem_str + i);
            write(ch);
        }
        return size;
    }


};


#endif //MWOS3_STM32F103_MWOSNETTRANSMITTER_H
