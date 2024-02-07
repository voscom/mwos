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
 * Если задано MWOS_SEND_BUFFER_SIZE - отправляет через свой кольцевой буффер, если не задано - то напрямую в Stream соединения
 * #define MWOS_SEND_BUFFER_SIZE
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

#ifndef MWOS_CRYPT_TYPE
#define MWOS_CRYPT_TYPE 0
#endif

extern MWOS3 mwos;

class MWOSNetTransmitter : Print {
public:

#pragma pack(push,1)
    // для отправки данных
    uint8_t needSendFormat:1; // необходимо отправить формат контроллера
    uint8_t IsConnected:1; // подключено к сереру
    uint8_t needCrypt:1; // необходимо шифровать данные
    uint8_t finishFormat:1; // необходимо отправить окончание формата
    uint8_t reserve0:4;
    MWOSModuleBase * sendModule=NULL;
    MWOSParam * sendParam=NULL;
    MWOS_PARAM_INDEX_UINT sendParamIndex=0;
#ifdef MWOS_SEND_BUFFER_USE
    uint8_t sendBuffer[MWOS_SEND_BUFFER_SIZE]; // кольцевой буффер отправляемых данных
    uint16_t writeOffset; // текущая позиция для записи в буффер
    uint16_t sendOffset; // текущая позиция в буффере для отправки на сервер
#endif
    MW_CRC16 send_crc16;
    Stream * _stream;
#if (MWOS_CRYPT_TYPE==1)
    uint8_t maskKey[4];
    uint8_t stepCrypt; // текущий шаг шифрования
    uint8_t stepDecrypt; // текущий шаг дешифрования
#endif
#pragma pack(pop)

    void sendUpdate() {
#ifdef MWOS_SEND_BUFFER_USE
        if (writeOffset != sendOffset) { // отправим данные из буффера отправки
            while (writeOffset != sendOffset) {
                int16_t sizeSend=writeOffset-sendOffset;
                if (sizeSend<0) sizeSend=MWOS_SEND_BUFFER_SIZE-sendOffset;
                MW_LOG(F("write block: ")); MW_LOG_LN(sizeSend);
                if (_stream->write(&sendBuffer[sendOffset],sizeSend)==0) {
                    MW_LOG(F("write block error: ")); MW_LOG_LN(sizeSend);
                    return; // никак не отправляется
                }
                sendOffset+=sizeSend;
                if (sendOffset>=MWOS_SEND_BUFFER_SIZE) sendOffset-=MWOS_SEND_BUFFER_SIZE;
            }
            return;
        }
        writeOffset=0;
        sendOffset=0;
#endif
        if (needSendFormat) writeFormatController();
        else if (IsConnected) writeChangedModules();
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
        sendModule=NULL;
        sendParam=NULL;
        needSendFormat=true;
        finishFormat=false;
        MW_LOG(F("sendFormatController start: ")); MW_LOG_LN(this->availableForWrite());
    }

    /***
     * Если задано needSendFormat отправляет формат контроллера на сервер
     * следит за переполнением буффера отправки и отправляет продолжение, когда место появится
     */
    void writeFormatController() {
        if (!finishFormat && sendModule==NULL && sendParam==NULL) {
            // отправим данные хранилищ
            if (frameStoragesInf()==0) return; // не отправлено
            sendParamIndex=0;
            sendModule=(MWOSModuleBase *) mwos.child; // начнем отправку описания первого модуля
            return;
        }
        if (sendModule!=NULL) {
            if (sendParam==NULL) { // сначала отправим формат модуля
                if (frameModuleInf(sendModule)==0) return; // не отправлено
                sendParam=(MWOSParam *) sendModule->child;
                return;
            } else { // отправим формат параметров
                if (frameModuleParamInf(sendModule,sendParam)==0) return; // не отправлено
                sendParam=(MWOSParam *) sendParam->next;
                if (sendParam!=NULL && sendParam->unitType!=UnitType::PARAM) sendParam = NULL;
                if (sendParam==NULL) {
                    sendModule=(MWOSModuleBase *) sendModule->next;
                    if (sendModule!=NULL && sendModule->unitType!=UnitType::MODULE) {
                        sendModule=NULL;
                        finishFormat=true;
                    }
                }
                return;
            }
        }
        if (finishFormat) { // отправим основные данные о прошивке контроллера
            if (frameFirmwareInf() == 0) return; // не отправлено
            finishFormat=false;
        }
        ToModule(mwos.child);
        needSendFormat=false;
        MW_LOG_LN(F("sendFormatController stop!"));
    }

    /**
     * Отправляет все измененные параметры на сервер отдельными блоками
     * следит за переполнением буффера отправки и отправляет продолжение, когда место появится
     */
    void writeChangedModules() {
        if (sendModule!=NULL) {
            if (sendModule->changedMask.haveSetBits()) { // только, если есть установленные биты изменения параметров для модуля
                if (sendParam!=NULL) {
                    if (!sendParam->IsGroup(mwos_param_secret)) {
                        if (sendParamIndex==0 && sendModule->IsParamAllChanged(sendParam)) { // отправим все значения
                            if (frameModuleParamAllValues(sendModule, sendParam) < 1) return; // нет места для отправки блока
                            MW_LOG(F("sendAllValues: ")); MW_LOG(sendModule->id); MW_LOG(':'); MW_LOG_LN(sendParam->id);
                            sendModule->SetParamChanged(sendParam, UINT16_MAX, false); // сбросим биты отправки
                        } else {
                            if (sendParamIndex < sendParam->arrayCount()) {
                                if (sendModule->IsParamChanged(sendParam, sendParamIndex)) {
                                    if (frameModuleParamValue(sendModule, sendParam, sendParamIndex) <1) return; // нет места для отправки блока
                                    MW_LOG(F("sendValues: ")); MW_LOG(sendModule->id); MW_LOG(':'); MW_LOG(sendParam->id); MW_LOG(':'); MW_LOG_LN(sendParamIndex);
                                    sendModule->SetParamChanged(sendParam, sendParamIndex, false); // сбросим бит отправки
                                }
                                sendParamIndex++;
                                return;
                            }
                        }
                    }
                    ToParam(sendParam->next); // следующий параметр
                    return;
                }
            }
            ToModule(sendModule->next); // следующий модуль
            return;
        }
        ToModule(mwos.child); // к первому модулю
    }

    void ToModule(MWOSUnit * toUnit) {
        sendModule=(MWOSModuleBase *) toUnit;
        if (sendModule!=NULL && sendModule->unitType!=UnitType::MODULE) sendModule=NULL;
        if (sendModule!=NULL) ToParam(sendModule->child);
        else ToParam(NULL);
    }

    void ToParam(MWOSUnit * toUnit) {
        sendParam=(MWOSParam *) toUnit;
        if (sendParam!=NULL && sendParam->unitType!=UnitType::PARAM) sendParam=NULL;
        sendParamIndex=0;
    }


    uint16_t beginFrame(MWOSProtocolCommand cmd, uint16_t moduleId=0, uint16_t paramId=0, uint16_t arrayIndex=0) {
#if (MWOS_CRYPT_TYPE==1)
        if (maskKey[0]!=0) {
            needCrypt= true; // начнем шифрование, если задана маска с ключом шифрования
            stepCrypt=0;
        }
#endif
        write(cmd);
        if (cmd>127) return 1;
        writeUInt16(moduleId);
        if (cmd>63) return 3;
        writeUInt16(paramId);
        if (cmd>31) return 5;
        writeUInt16(arrayIndex);
        return 7;
    }

    /***
     * Отправить признак начала пакета для сервера
     * @return Размер отправленного
     */
    uint16_t beginPacket() {
#if (MWOS_CRYPT_TYPE==1)
        needCrypt= false; // подпись начала пакета не шифруем
        stepCrypt=0;
#endif
        write(02); // 1 признак начала пакета
        writePacketSign(); // 4 подпись пакета
        send_crc16.start();
        return 4;
    }


    /***
     * Отправить признак окончания пакета для сервера
     * @return Размер отправленного
     */
    uint16_t endPacket() {
        needCrypt= false; // подпись конца пакета и контрольную сумму не шифруем
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
#if (MWOS_CRYPT_TYPE==1)
        if (needCrypt) {
            byte^=maskKey[stepCrypt++];
            stepCrypt &=3;
        }
#endif
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

/*************** ОТПРАВКА ФРЕЙМОВ ********************/

    /***
     * Отправить на сервер фрейм Дополнительные данные в формате JSON
     * @return  Размер отправленного
     * @return
     */
    size_t frameJSON(char * jsonInFlash) {
        int frameSize=strlen_P(jsonInFlash)+12;
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameJSON error: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        int size=beginPacket();
        // 0: Код фрейма: Дополнительные данные в формате JSON
        size+=beginFrame(mwos_server_cmd_frame_json);
        // 1: Дополнительные данные в формате JSON
        size+=write_progmem_str(jsonInFlash);
        size+=write(0); // 2 конец строки
        size+=endPacket();
        MW_LOG(F("frameJSON: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }

    /***
     * Отправить на сервер фрейм хендшейка
     * @return  Размер отправленного
     */
    size_t frameHandshake(uint32_t cid) {
        int frameSize=strlen_P((char *) mwos_project);
        frameSize+=40;
#if (MWOS_CRYPT_TYPE==1)
        frameSize+=4; // учтем размер ключа
        maskKey[0]=0; // что-бы не шифровало начало фрейма
#endif
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameHandshake error: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        int size=beginPacket();
        // 0: Код фрейма: HANDSHAKE от клиента серверу
        size+=beginFrame(mwos_server_cmd_handshake);
        // 1: генерируемый ключ для шифра сессии (далее - все шифруется)
        size+=write(MWOS_CRYPT_TYPE); // тип шифрования
#if (MWOS_CRYPT_TYPE==1)
        MW_LOG(F("session maskKey: "));
        for(uint8_t x = 0; x < sizeof(maskKey); x++) {
            maskKey[x] = random(1,0xFF);
            size+=write(maskKey[x]); // отправим байт ключа сессии без учета секретного ключа
            MW_LOG('-'); MW_LOG(maskKey[x]);
        }
        MW_LOG_LN();
        stepCrypt=0;
        needCrypt= true; // начнем шифрование
#endif
        // 6: 64-бит хеш пользователя с сайта MWOS
        size+=writeUInt64(MWOS_USER_HASH);
        // 14: Глобальный ID контроллера CID (если 0 - используется LID)
        size+=writeUInt32(cid);
        // 18: Локальный ID контроллера (LID) у пользователя (или chipID, если LID=0)
        uint32_t localID=MWOS_CONTROLLER_ID;  // локальный ID контроллера у пользователя
        if (localID==0) localID=getChipID(); // если LID=0, то chipID
        size+=writeUInt32(localID);
        // 22: Тип контроллера
        size+=write(MWOS_CONTROLLER_TYPE);
        // 23: Количество модулей контроллера
        size+=writeUInt16(mwos.modulesCount);
        // 25: Время компиляции прошивки (UNIXTIME)
        uint64_t timeStampBuild=BUILD_TIME_PROJ;
        size+=write((uint8_t *) &timeStampBuild,8); // еще 8 байт - время компиляции
        // 33: Название проекта
        size+=write_progmem_str((char *) mwos_project); // имя проекта
        size+=write(0); // 34 конец строки
        size+=endPacket();
#if (MWOS_CRYPT_TYPE==1)
        MW_LOG(F("real maskKey: "));
        uint8_t * mwos_secret_key_link= (uint8_t *) mwos_secret_key;
        for(uint8_t i = 0; i < sizeof(maskKey); i++) {
            maskKey[i] ^= pgm_read_byte_near(mwos_secret_key_link + i); // учтем секретный ключ
            MW_LOG('-'); MW_LOG(maskKey[i]);
        }
        MW_LOG_LN();
#endif
        MW_LOG(F("frameHandshake: ")); MW_LOG(cid); MW_LOG(':'); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN((int) size);
        return size;
    }

protected:

    /**
     * Отправить на сервер фрейм данных о прошивке
     * @return Размер отправленного
     */
    size_t frameFirmwareInf() {
        int frameSize=strlen_P((char *) mwos_board);
        frameSize+=mwos.nameSize();
        frameSize+=strlen_P((char *) git_hash_proj);
        frameSize+=strlen_P((char *) git_url_proj);
        frameSize+=strlen_P((char *) git_hash_mwos);
        frameSize+=strlen_P((char *) git_url_mwos);
        frameSize+=17;
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameFirmwareInf error: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        // 0: Код фрейма: Данные о прошивке
        int size=beginPacket();
        size+=beginFrame(mwos_server_cmd_frame_firmware);
        // 1: Название контроллера (или JSON с информацией)
        size+=mwos.printName(this);
        size+=write(0); // конец строки
        // 2: Имя платформы board из platformio.ini
        size+=write_progmem_str((char *) mwos_board);
        size+=write(0); // конец строки
        // 3: GIT хеш проекта
        size+=write_progmem_str((char *) git_hash_proj);
        size+=write(0); // конец строки
        // 4: GIT url проекта
        size+=write_progmem_str((char *) git_url_proj);
        size+=write(0); // конец строки
        // 5: GIT хеш MWOS
        size+=write_progmem_str((char *) git_hash_mwos);
        size+=write(0); // конец строки
        // 6: GIT url MWOS
        size+=write_progmem_str((char *) git_url_mwos);
        size+=write(0); // 97 конец строки
        size+=endPacket();
        MW_LOG(F("frameFirmwareInf: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }

    /**
     * Отправить на сервер фрейм данных о хранилищах
     * @return Размер отправленного
     */
    size_t frameStoragesInf() {
        int frameSize=12;
        for (uint8_t i = 0; i < mwos.storageCount; ++i) {
            MWOSStorage * storage=mwos.storages[i];
            frameSize+=storage->nameSize()+9;
        }
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameStoragesInf error: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        // 0: Код фрейма: Данные о хранилищах
        int size=beginPacket();
        size+=beginFrame(mwos_server_cmd_frame_storages);
        // 1: количество хранилищ
        size+=write(mwos.storageCount);
        // далее - данные на каждое хранилище
        for (uint8_t i = 0; i < mwos.storageCount; ++i) {
            MWOSStorage * storage=mwos.storages[i];
            // 0: размер хранилища
            size+=writeUInt32(storage->_bitSize);
            // 4: смещение хранилища
            size+=writeUInt32(storage->_offset);
            // 8: Имя хранилища или JSON с информацией
            size+=storage->printName(this);
            size+=write(0); // 9: конец строки
        }
        size+=endPacket();
        MW_LOG(F("frameStoragesInf: ")); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }

    /**
     * Отправить на сервер фрейм формат модуля
     * @return Размер отправленного
     */
    size_t frameModuleInf(MWOSModuleBase * module) {
        int frameSize=20;
        frameSize+=module->nameSize();
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameModuleInf error: ")); MW_LOG_PROGMEM(module->name); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        // 0: Код фрейма: Данные о модуле
        int size=beginPacket();
        size+=beginFrame(mwos_server_cmd_module_inf,module->id);
        // 3: Количество параметров в модуле
        size+=writeUInt16(module->paramsCount);
        // 5: Тип модуля
        size+=write(module->moduleType);
        // 6: маска хранилищ, куда уже сохранены данные параметров модуля
        uint8_t storagesInited=module->storage0Init;
        if (module->storage1Init) storagesInited |=2;
        if (module->storage2Init) storagesInited |=4;
        if (module->storage3Init) storagesInited |=8;
        size+=write(storagesInited);
        // 7: Номер параметра с таким именем
        size+=writeUInt16(mwos.getNumForName(module));
        // 9: Имя модуля или JSON с информацией
        size+=module->printName(this); // имя (или данные в формате json)
        size+=write(0); // 8 конец строки
        size+=endPacket();
        MW_LOG(F("frameModuleInf ")); MW_LOG_PROGMEM(module->name); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }

    /**
     * Отправить на сервер фрейм формат параметра модуля
     * @return Размер отправленного
     */
    size_t frameModuleParamInf(MWOSModuleBase * module, MWOSParam * param) {
        int frameSize=21;
        frameSize+=param->nameSize();
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameModuleParamInf error: ")); MW_LOG_PROGMEM(module->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        int size=beginPacket();
        // 0: Код фрейма: Данные о параметре модуля
        size+=beginFrame(mwos_server_cmd_param_inf,module->id,param->id);
        // 5: Количество значений параметра (размер массива)
        size+=writeUInt16(sendParam->arrayLength);
        // 7: Тип значения ParamValueType
        size+=write(sendParam->valueType);
        // 8: Группа параметра  ParamGroup
        size+=write(sendParam->group);
        // 9: Где хранится (хранилище)
        size+=write((uint8_t) sendParam->storage);
        // 10: Имя параметра или JSON с информацией
        size+=sendParam->printName(this); // имя (или данные в формате json)
        size+=write(0); // 11 конец строки
        size+=endPacket();
        MW_LOG(F("frameModuleParamInf ")); MW_LOG_PROGMEM(module->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG('='); MW_LOG(module->id); MW_LOG(':'); MW_LOG(param->id); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }

    /**
     * Отправить на сервер фрейм Значение параметра
     * @return Размер отправленного
     */
    size_t frameModuleParamValue(MWOSModuleBase * module, MWOSParam * param, uint16_t index) {
        int frameSize=17;
        uint16_t byteSize=param->byteSize(false);
        frameSize+=byteSize;
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameModuleParamValue error: ")); MW_LOG_PROGMEM(module->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG(':'); MW_LOG(index); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        // 0: Код фрейма: Данные о параметре модуля
        int size=beginPacket();
        size+=beginFrame(mwos_server_cmd_param_set_value,module->id,param->id, index);
        // 7:Значение параметра (длина данных значения зависит от типа параметра)
        int64_t value= module->getValue(param, index);
        uint8_t * block=(uint8_t *) &value;
        for (uint16_t i = 0; i < byteSize; ++i) {
            size+=write(block[i]);
        }
        size+=endPacket();
        MW_LOG(F("frameModuleParamValue ")); MW_LOG_PROGMEM(module->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG(':'); MW_LOG(index); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }

    /**
     * Отправить на сервер фрейм Все значения параметра
     * @return Размер отправленного
     */
    size_t frameModuleParamAllValues(MWOSModuleBase * module, MWOSParam * param) {
        int frameSize=15;
        uint16_t byteSize=param->byteSize(false);
        frameSize+=byteSize*param->arrayCount();
        if (availableForWrite()<frameSize) { // нет места для этого фрейма
            MW_LOG(F("frameModuleParamAllValues error: ")); MW_LOG_PROGMEM(module->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(availableForWrite());
            return 0;
        }
        // 0: Код фрейма: Данные о параметре модуля
        int size=beginPacket();
        size+=beginFrame(mwos_server_cmd_param_set_param_all_values,module->id,param->id);
        // 5:Значение параметра (длина данных значения зависит от типа параметра)
        if (param->IsLong()) {
            PackValueArrayPrt v;
            v.value= module->getValue(param, 0);
            if (v.size>0 && v.addr>0) {
                uint8_t * block=(uint8_t *) v.addr;
                for (uint16_t i = 0; i < v.size; i++) {
                    size+=write(block[i]);
                }
            }
        } else {
            for (int index = 0; index < param->arrayCount(); index++) {
                int64_t value= module->getValue(param, index);
                uint8_t * block=(uint8_t *) &value;
                for (uint16_t i = 0; i < byteSize; i++) {
                    size+=write(block[i]);
                }
            }
        }
        size+=endPacket();
        MW_LOG(F("frameModuleParamAllValues ")); MW_LOG_PROGMEM(module->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG('='); MW_LOG(frameSize); MW_LOG('/'); MW_LOG_LN(size);
        return size;
    }


};


#endif //MWOS3_STM32F103_MWOSNETTRANSMITTER_H
