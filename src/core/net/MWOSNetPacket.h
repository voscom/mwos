#ifndef MWOS3_MWOSNETPACKET_H
#define MWOS3_MWOSNETPACKET_H
#include "MWOSNetPrint.h"

/**
 * Формат отправляемых пакетов и фреймов
 */
class MWOSNetPacket : public MWOSNetPrint {
public:

    String valueStr="";

    /***
     * Отправить признак начала пакета для сервера
     * @param dataSize  Размер данных фрейма (включая команду и аргументы)
     * @return Размер отправленного
     */
    uint16_t beginPacket(uint16_t dataSize) {
#if (MWOS_CRYPT_TYPE==1)
        needCrypt= false; // подпись начала пакета не шифруем
        stepCrypt=0;
#endif
        uint16_t siz=0;
        siz+=write((uint8_t) 04); // 1 признак начала пакета
        siz+=write((uint8_t) 'M'); // признак пакета
        siz+=write((uint8_t) 'W'); // признак пакета
        siz+=write((uint8_t) 03); // признак пакета
        send_crc16.start();
        siz+=writeUInt16(dataSize);
        return siz;
    }


    /***
     * Отправить признак окончания пакета для сервера
     * @return Размер отправленного
     */
    uint16_t endPacket() {
        needCrypt= false; // подпись конца пакета и контрольную сумму не шифруем
        // отправим контрольную сумму
        return writeUInt16(send_crc16.getCRC()); // 2
    }

    uint16_t beginFrame(MWOSProtocolCommand cmd, uint16_t moduleId, uint16_t paramId, uint16_t arrayIndex) {
#if (MWOS_CRYPT_TYPE==1)
        if (maskKey[0]!=0) {
            needCrypt= true; // начнем шифрование, если задана маска с ключом шифрования
            stepCrypt=0;
        }
#endif
        uint16_t siz=0;
        siz+=write((uint8_t) cmd);
        if (cmd>127) return siz;
        siz+=writeUInt16(moduleId);
        if (cmd>63) return siz;
        siz+=writeUInt16(paramId);
        if (cmd>31) return siz;
        siz+=writeUInt16(arrayIndex);
        return siz;
    }


    /**
     * Локальный ID контроллеры
     * @return
     */
    uint32_t getLID() {
        //uint32_t localID=MWOS_CONTROLLER_ID;  // локальный ID контроллера у пользователя
        //if (localID==0) localID=getChipID(); // если LID=0, то chipID
        return MWOS_CONTROLLER_ID;
    }

/*************** ОТПРАВКА ФРЕЙМОВ ********************/

    /***
     * Отправить на сервер фрейм Дополнительные данные в формате JSON
     * @return  Размер отправленного
     * @return
     */
    size_t frameJSON(char * jsonInFlash) {
        uint16_t frameSize=strlen_P(jsonInFlash)+2;
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_TIME(); MW_LOG(F("frameJSON out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        size_t size=beginPacket(frameSize);
        // 0: Код фрейма: Дополнительные данные в формате JSON
        size+=beginFrame(mwos_server_cmd_frame_json,0,0,0); // 1
        // 1: Дополнительные данные в формате JSON
        size+=write_progmem_str(jsonInFlash); // 2
        size+=endPacket();
#if (MWOS_LOG_SEND>0)
        MW_LOG_TIME(); MW_LOG(F("jsonInFlash")); MW_LOG_LN((uint16_t) size);
#endif
        if (frameSize+7!=(uint16_t) size) {
#if (MWOS_LOG_SEND>=0)
            MW_LOG_TIME(); MW_LOG(F("frameJSON error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) size);
#endif
        }
        return size;
    }

    /**
     * Отправить на сервер одну команду (пакет из одного байта)
     * @return  Размер отправленного
     */
    size_t frameSendCommand(MWOSProtocolCommand sendCommand) {
        if (availableForWrite()<9) {
#if (MWOS_LOG_SEND>=0)
            MW_LOG_TIME(); MW_LOG(F("frameSendCommand ")); MW_LOG(9); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        size_t size=beginPacket(1); // 6
        size+=beginFrame(sendCommand,0,0,0); // 1
        size+=endPacket(); //2
        return size;
    }

    /***
     * Отправить на сервер фрейм хендшейка
     * @return  Размер отправленного
     */
    size_t frameHandshake(uint32_t cid) {
        uint16_t frameSize=strlen_P((char *) mwos_project);
        frameSize+=34;
#if (MWOS_CRYPT_TYPE==1)
        frameSize+=4; // учтем размер ключа
        maskKey[0]=0; // что-бы не шифровало начало фрейма
#endif
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_TIME(); MW_LOG(F("frameHandshake out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        size_t size=beginPacket(frameSize);
        // 0: Код фрейма: HANDSHAKE от клиента серверу
        size+=beginFrame(mwos_server_cmd_handshake,0,0,0); // 1
        // 1: генерируемый ключ для шифра сессии (далее - все шифруется)
        size+=write((uint8_t) MWOS_CRYPT_TYPE); // 2 тип шифрования
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
        size+=writeUInt64(MWOS_USER_HASH); // 10
        // 14: Глобальный ID контроллера CID (если 0 - используется LID)
        size+=writeUInt32(cid); // 14
        // 18: Локальный ID контроллера (LID) у пользователя (или chipID, если LID=0)
        size+=writeUInt32(getLID()); // 18  локальный ID контроллера у пользователя
        // 22: Тип контроллера
        size+=write((uint8_t) MWOS_CONTROLLER_TYPE); // 19
        // 23: Количество модулей контроллера
        size+=writeUInt16(mwos.modulesCount); // 21
        // 25: Время компиляции прошивки (UNIXTIME)
        size+=writeUInt64((uint64_t) MWOS_BUILD_TIME); // 29 время компиляции
        // 33: Название проекта
        size+=write_progmem_str((char *) mwos_project); // 30 имя проекта
        size+=writeUInt32(getChipID()); // 31 + 4 бай ID чипа
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
#if (MWOS_LOG_SEND>0)
        MW_LOG_TIME(); MW_LOG(F("frameHandshake")); MW_LOG_LN((uint16_t) size);
#endif
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=(uint16_t) size) {
            MW_LOG_TIME(); MW_LOG(F("frameHandshake error ")); MW_LOG(cid); MW_LOG(':'); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((int) size);
        }
#endif
        return size;
    }

    /**
     * Отправить на сервер фрейм данных о прошивке
     * @return Размер отправленного
     */
    size_t frameFirmwareInf() {
        uint16_t frameSize=mwos.nameSize();
        frameSize+=strlen_P((char *) mwos_board);
        frameSize+=strlen_P((char *) mwos_board_full);
        frameSize+=strlen_P((char *) mwos_libs);
        frameSize+=7;
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_TIME(); MW_LOG(F("frameFirmwareInf out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // 0: Код фрейма: Данные о прошивке
        size_t size=beginPacket(frameSize);
        size+=beginFrame(mwos_server_cmd_frame_firmware,0,0,0); // 1
        // 1: Название контроллера (или JSON с информацией)
        size+=write_progmem_str((char *) mwos.name); // 2
        // 2: Имя платформы board из platformio.ini
        size+=write_progmem_str((char *) mwos_board); // 3
        // 3: Полное имя платформы board из platformio.ini
        size+=write_progmem_str((char *) mwos_board_full); // 4
        // 4: GIT версии проекта и всех библиотек (в формате JSON)
        size+=write_progmem_str((char *) mwos_libs); // 5
        size+=writeUInt16(MWOS_RECEIVE_BLOCK_SIZE); // 7 размер буффера приема
        size+=endPacket();
#if (MWOS_LOG_SEND>0)
        MW_LOG(F("fwInf")); MW_LOG((uint16_t) size); MW_LOG(';');
#endif
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=(uint16_t) size) {
            MW_LOG_TIME(); MW_LOG(F("frameFirmwareInf error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) size);
        }
#endif
        return size;
    }

    /**
     * Отправить на сервер фрейм данных о хранилищах
     * @return Размер отправленного
     */
    size_t frameStoragesInf() {
        uint16_t frameSize=2;
        for (uint8_t i = 0; i < MWOSStorage::storagesCount; ++i) {
            MWOSStorage * storage=mwos.storages[i];
            frameSize+=storage->nameSize()+9;
        }
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_TIME(); MW_LOG(F("frameStoragesInf out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // 0: Код фрейма: Данные о хранилищах
        uint16_t psize=beginPacket(frameSize);
        psize+=beginFrame(mwos_server_cmd_frame_storages,0,0,0); // 1
        // 1: количество хранилищ
        psize+=write((uint8_t) MWOSStorage::storagesCount); // 2
        // далее - данные на каждое хранилище
        for (uint8_t i = 0; i < MWOSStorage::storagesCount; ++i) {
            MWOSStorage * storage=mwos.storages[i];
            // 0: размер хранилища
            psize+=writeUInt32(storage->_bitSize); // 4
            // 4: смещение хранилища
            psize+=writeUInt32(storage->_offset); // 8
            // 8: Имя хранилища или JSON с информацией
            psize+=write_progmem_str((char *) storage->name); // 9
        }
        psize+=endPacket();
#if (MWOS_LOG_SEND>0)
        MW_LOG(F("sInf")); MW_LOG((uint16_t) MWOSStorage::storagesCount); MW_LOG('='); MW_LOG((uint16_t) psize); MW_LOG(';');
#endif
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=psize) {
            MW_LOG_TIME(); MW_LOG(F("frameStoragesInf error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) psize);
        }
#endif
        return psize;
    }

    /**
     * Отправить на сервер фрейм формат модуля
     * @return Размер отправленного
     */
    size_t frameModuleInf(MWOSModuleBase * module) {
        uint16_t frameSize=10;
        frameSize+=module->nameSize();
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module); MW_LOG(F("frameModuleInf out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // Код фрейма: Данные о модуле
        size_t size=beginPacket(frameSize);
        size+=beginFrame(mwos_server_cmd_module_inf,module->id,0,0); // 3
        // Количество параметров в модуле
        size+=writeUInt16(module->GetParamCount(true)); // 5
        // Тип модуля
        size+=write((uint8_t) module->moduleType); // 6
        // маска хранилищ, куда уже сохранены данные параметров модуля
        size+=write((uint8_t) 0); // 7 пока зарезервируем
        // Номер параметра с таким именем
        size+=writeUInt16(mwos.getNumForName(module)); // 9
        // Имя модуля или JSON с информацией
        size+=write_progmem_str((char *) module->name); // 10
        size+=endPacket();
#if (MWOS_LOG_SEND>3)
         MW_LOG(F("mInf")); MW_LOG((uint16_t) module->id); MW_LOG('='); MW_LOG((uint16_t) size); MW_LOG(';');
#endif
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=(uint16_t) size) {
            MW_LOG_MODULE(module); MW_LOG(F("frameModuleInf error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) size);
        }
#endif
        return size;
    }

    /**
     * Отправить на сервер фрейм формат параметра модуля
     * @return Размер отправленного
     */
    size_t frameModuleParamInf(MWOSModuleBase * module, MWOSParam * param) {
        uint16_t frameSize=11;
        frameSize+=param->nameSize();
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module,param); MW_LOG(F("frameModuleParamInf out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        //uint16_t beginPacketOffset=writeOffset; // тесты!!!
        int size=beginPacket(frameSize);
        // 0: Код фрейма: Данные о параметре модуля
        size+=beginFrame(mwos_server_cmd_param_inf,module->id,param->id,0); // 5
        // 5: Количество значений параметра (размер массива)
        size+=writeUInt16(param->arrayLength); // 7
        // 7: Тип значения ParamValueType
        size+=write((uint8_t) param->valueType); // 8
        // 8: Группа параметра  ParamGroup
        size+=write((uint8_t) param->group); // 9
        // 9: Где хранится (хранилище)
        uint8_t storS=param->getStorage();
        if (storS < MWOS_STORAGE_NO && param->storageInit) storS+=128; // признак, что в хранилище параметра уже не данные по умолчанию
        size+=write((uint8_t) storS); // 10
        // 10: Имя параметра или JSON с информацией
        size+=write_progmem_str((char *) param->name); // 11
        size+=endPacket();
#if (MWOS_LOG_SEND>3)
        MW_LOG(F("pInf")); MW_LOG((uint16_t) module->id); MW_LOG(':'); MW_LOG((uint16_t) param->id); MW_LOG('='); MW_LOG((uint16_t) size); MW_LOG(';');
#endif
        //if (module->id==19 && param->id==0) { MW_LOG_MODULE(module,param); MW_LOG(F("sendInf ")); MW_LOG_BYTES(sendBuffer+beginPacketOffset,size,24,DEC); MW_LOG_LN(); }
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=size) {
            MW_LOG_MODULE(module,param); MW_LOG(F("frameModuleParamInf error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) size);
        }
#endif
        return size;
    }

    /**
     * Отправить на сервер фрейм Значение параметра
     * @return Размер отправленного
     */
    size_t frameModuleParamValue(MWOSModuleBase * module, MWOSParam * param, uint16_t index) {
        uint16_t frameSize=7;
        uint16_t byteSize=param->byteSize(false);
        frameSize+=byteSize;
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module,param,index); MW_LOG(F("frameModuleParamValue out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // места хватает - отправляем
        module->SetParamChanged(param,index, false); // сбросим флаг отправки
        // 0: Код фрейма: Данные о параметре модуля
        uint16_t wsize=beginPacket(frameSize);
        wsize+=beginFrame(mwos_server_cmd_param_set_value,module->id,param->id, index); // 7
        // 7:Значение параметра (длина данных значения зависит от типа параметра)
        int64_t value= module->getValue(param, index);
        uint8_t * block=(uint8_t *) &value;
#if (MWOS_LOG_SEND>3)
        MW_LOG_MODULE(module,param); MW_LOG(F("SendV:"));
#endif

        for (uint16_t i = 0; i < byteSize; ++i) {
            wsize+=write((uint8_t) block[i]);
#if (MWOS_LOG_SEND>3)
            MW_LOG_BYTE(block[i],' ');
#endif
        }
#if (MWOS_LOG_SEND>3)
        MW_LOG('>'); MW_LOG_LN(frameSize-7);
#endif
        wsize+=endPacket();
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=wsize) {
            MW_LOG_MODULE(module,param,index); MW_LOG(F("frameModuleParamValue error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN(wsize);
        }
#endif
        return wsize;
    }

    /**
     * Отправить на сервер фрейм Все значения параметра
     * @return Размер отправленного
     */
    size_t frameModuleParamAllValues(MWOSModuleBase * module, MWOSParam * param) {
        uint16_t frameSize=5;
        uint16_t byteSize=param->byteSize(false);
        uint16_t arrayCount=param->arrayCount();
        if (param->IsString()) { // строка имеет динамическую длину
            if (valueStr.isEmpty()) {
                if (arrayCount<1) arrayCount=MWOS_SEND_BUFFER_SIZE-128; // если строка = 0, то это вообще любое допустимое значение
                for (MWOS_PARAM_INDEX_UINT i = 0; i < arrayCount; i++) { // длинные параметры (string и byteArray) извлекаем побайтно
                    uint8_t b=(uint8_t) module->getValue(param, i);
                    if (b == 0) break; // строка заканчивается нулем
                    valueStr+=(char) b;
                }
            }
            arrayCount=valueStr.length()+1;
            if (arrayCount<1) arrayCount=1;
            byteSize=1;
        }
        frameSize+=byteSize*arrayCount;
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module,param); MW_LOG(F("frameModuleParamAllValues out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // места хватает - отправляем
        module->SetParamChanged(param,UINT16_MAX, false); // сбросим флаги отправки
        // 0: Код фрейма: Данные о параметре модуля
        uint16_t wsize=beginPacket(frameSize);
        wsize+=beginFrame(mwos_server_cmd_param_set_param_all_values,module->id,param->id,0); // 5
        // 5:Значение параметра (длина данных значения зависит от типа параметра)
#if (MWOS_LOG_SEND>3)
        MW_LOG_MODULE(module,param); MW_LOG(F("SendAV:"));
#endif
        if (param->IsLong()) {
            bool isString=param->IsString();
            for (MWOS_PARAM_INDEX_UINT i = 0; i < arrayCount; i++) { // длинные параметры (string и byteArray) извлекаем побайтно
                uint8_t b;
                if (isString) {
                    if (valueStr.length()>i) b=(uint8_t) valueStr[i];
                    else b=0;
                } else
                    b=(uint8_t) module->getValue(param, i);
                wsize+=write((uint8_t) b);
                //MW_LOG_BYTE(b,' ');
                if (b == 0 && isString) {
                    // прочитаем еще один байт за пределами строки
                    // (модули могут это отслеживать для перехода к следующей строке)
                    module->getValue(param, i+3);
                    break; // строка заканчивается нулем
                }
            }
            valueStr="";
        } else {
            for (int index = 0; index < arrayCount; index++) {
                int64_t value= module->getValue(param, index);
                uint8_t * block=(uint8_t *) &value;
                for (uint16_t i = 0; i < byteSize; i++) {
                    wsize+=write((uint8_t) block[i]);
#if (MWOS_LOG_SEND>3)
                    MW_LOG_BYTE(block[i],' ');
#endif
                }
            }
        }
        wsize+=endPacket();
#if (MWOS_LOG_SEND>3)
        MW_LOG('>'); MW_LOG_LN(frameSize-5);
#endif
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=wsize) {
            MW_LOG_MODULE(module,param); MW_LOG(F("frameModuleParamAllValues error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN(wsize);
        }
#endif
        return wsize;
    }

    /**
     * Отправить на сервер массив данных в фрейме "Все значения параметра"
     * @param module
     * @param param
     * @param buff
     * @param frameSize
     * @return  Размер отправленного
     */
    size_t frameModuleParamAllValuesByteArray(MWOSModuleBase * module, MWOSParam * param, const uint8_t * buff, uint16_t dataSize) {
        uint16_t frameSize=dataSize+5;
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module,param); MW_LOG(F("frameModuleParamAllValuesByteArray out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // места хватает - отправляем
        module->SetParamChanged(param,UINT16_MAX, false); // сбросим флаги отправки
        // 0: Код фрейма: Данные о параметре модуля
        size_t size=beginPacket(frameSize);
        size+=beginFrame(mwos_server_cmd_param_set_param_all_values,module->id,param->id,0); // 5
        size+=write(buff,frameSize);
        size+=endPacket();
        if (frameSize+8!=(uint16_t) size) {
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module,param);  MW_LOG(F("frameModuleParamAllValuesByteArray error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) size);
#endif
        }
        return size;
    }

    size_t frameModuleParamValueByteArray(MWOSModuleBase * module, MWOSParam * param, uint16_t index, const uint8_t * buff, uint16_t dataSize) {
        uint16_t frameSize=dataSize+7;
        if (availableForWrite()<frameSize+16) { // нет места для этого фрейма
#if (MWOS_LOG_SEND>=0)
            MW_LOG_MODULE(module,param,index); MW_LOG(F("frameModuleParamValueByteArray out _buff ")); MW_LOG(frameSize+16); MW_LOG('/'); MW_LOG_LN(availableForWrite());
#endif
            return 0;
        }
        // места хватает - отправляем
        module->SetParamChanged(param,UINT16_MAX, false); // сбросим флаги отправки
        // 0: Код фрейма: Данные о параметре модуля
        size_t size=beginPacket(frameSize);
        size+=beginFrame(mwos_server_cmd_param_set_value,module->id,param->id,index); // 7
        size+=write(buff,frameSize);
        size+=endPacket();
#if (MWOS_LOG_SEND>=0)
        if (frameSize+8!=(uint16_t) size) {
            MW_LOG_MODULE(module,param,index);  MW_LOG(F("frameModuleParamValueByteArray error ")); MW_LOG(frameSize+8); MW_LOG('/'); MW_LOG_LN((uint16_t) size);
        }
#endif
        return size;
    }

};


#endif //MWOS3_MWOSNETPACKET_H
