#ifndef MWNETFRAMESEND_H
#define MWNETFRAMESEND_H
#include <Arduino.h>
#include "core/net/MWNetPrint.h"

#ifndef MWOS_CONTROLLER_TYPE
#define MWOS_CONTROLLER_TYPE 0
#endif

#ifndef MWOS_USER_HASH
#define MWOS_USER_HASH 0
#endif

/**
 * Создает фреймы для отправки на сервер. Напрямую в буфере отправки. Минимально возможный расход памяти.
 */
class MWNetFrameSend : public MWNetPrint {
public:

    MWNetPacketReceiver * _net;

    /**
     * Отправить на сервер фрейм хендшейка
     * @return  Размер отправленного
     */
    size_t SendHandshake() {
        frameStart();
        write(mwos_server_cmd_handshake);
#ifdef MWOS_CRYPT_KEY
        write(1); // тип шифрования AES 128 CRT
#else
        write(0); // тип шифрования - нет
#endif
        // 6: 64-бит хеш пользователя с сайта MWOS
        writeUInt64(MWOS_USER_HASH);
        // 14: Глобальный ID контроллера CID (если 0 - используется LID)
        writeUInt32(mwos.getCID());
        // 18: Локальный ID контроллера (LID) у пользователя (или chipID, если LID=0)
        writeUInt32(mwos.getLID());
        // 22: Тип контроллера
        write((uint8_t) MWOS_CONTROLLER_TYPE);
        // 23: Количество модулей контроллера
        writeUInt16(mwos.modulesCount);
        // 25: Время компиляции прошивки (UNIXTIME)
        writeUInt64((uint64_t) MWOS_BUILD_TIME);
        // 33: Название проекта
        write_progmem_str((char *) mwos_project);
        // 33: ID чипа
        writeUInt32(getChipID());
#if LOG_SEND>3
        MW_LOG_TIME(); MW_LOG_LN((char *) F("FrameHandshake"));
#endif
        frameEnd();
        return _size;
    }

    /**
     * Отправить на сервер фрейм данных о прошивке
     * @return Размер отправленного
     */
    size_t SendFirmwareInf() {
        frameStart();
        beginFrame(mwos_server_cmd_frame_firmware,0,0,0); // 1
        // 1: Название контроллера (или JSON с информацией)
        write_progmem_str((char *) mwos.name);
        // 2: Имя платформы board из platformio.ini
        write_progmem_str((char *) mwos_board); // 3
        // 3: Полное имя платформы board из platformio.ini
        write_progmem_str((char *) mwos_board_full); // 4
        // 4: GIT версии проекта и всех библиотек (в формате JSON)
        write_progmem_str((char *) mwos_libs); // 5
        writeUInt16(_net->receivePacket._sizeBuffer); // 7 размер буфера приема
#if LOG_SEND>3
        MW_LOG_TIME(); MW_LOG_LN((char *) F("frameFirmwareInf"));
#endif
        frameEnd();
        return _size;
    }

    /**
     * Отправить на сервер фрейм данных о хранилищах
     * @return Размер отправленного
     */
    size_t SendStoragesInf() {
        frameStart();
        beginFrame(mwos_server_cmd_frame_storages,0,0,0); // 1
        // 1: количество хранилищ
        write((uint8_t) MWStorage::storagesCount); // 2
        // далее - данные на каждое хранилище
        for (uint8_t i = 0; i < MWStorage::storagesCount; ++i) {
            MWStorage * storage=mwos.storages[i];
            // 0: размер хранилища
            writeUInt32(storage->_bitSize); // 4
            // 4: смещение хранилища
            writeUInt32(storage->_offset); // 8
            // 8: Имя хранилища или JSON с информацией
            write_progmem_str((char *) storage->name); // 9
        }
#if LOG_SEND>3
        MW_LOG_TIME(); MW_LOG_LN((char *) F("frameStoragesInf"));
#endif
        frameEnd();
        return _size;
    }

    /**
 * Отправить на сервер фрейм формат модуля
 * @return Размер отправленного
 */
    size_t SendModuleInf(MWOSModuleBase * module) {
        if (!module) return 0;
        frameStart();
        beginFrame(mwos_server_cmd_module_inf,module->id,0,0); // 3
        // Количество параметров в модуле
        writeUInt16(module->GetParamCount(true)); // 5
        // Тип модуля
        write((uint8_t) module->moduleType); // 6
        // маска хранилищ, куда уже сохранены данные параметров модуля
        write((uint8_t) 0); // 7 пока зарезервируем
        // Номер параметра с таким именем
        writeUInt16(mwos.getNumForName(module)); // 9
        // Имя модуля или JSON с информацией
        write_progmem_str((char *) module->name); // 10
#if LOG_SEND>3
        MW_LOG_MODULE(module); MW_LOG_LN((char *) F("frameModuleInf"));
#endif
        frameEnd();
        return _size;
    }

    /**
     * Отправить на сервер фрейм формат параметра модуля
     * @return Размер отправленного
     */
    size_t SendModuleParamInf(MWOSModuleBase * module, MWOSParam * param) {
        if (!module || !param) return 0;
        frameStart();
        // 0: Код фрейма: Данные о параметре модуля
        beginFrame(mwos_server_cmd_param_inf,module->id,param->id,0); // 5
        // 5: Количество значений параметра (размер массива)
        writeUInt16(param->arrayLength); // 7
        // 7: Тип значения ParamValueType
        write((uint8_t) param->valueType); // 8
        // 8: Группа параметра  ParamGroup
        write((uint8_t) param->group); // 9
        // 9: Где хранится (хранилище)
        uint8_t storS=param->getStorage();
        //if (storS < MWOS_STORAGE_NO && param->storageInit) storS+=128; // признак, что в хранилище параметра уже не данные по умолчанию
        write((uint8_t) storS); // 10
        // 10: Имя параметра или JSON с информацией
        write_progmem_str((char *) param->name); // 11
#if LOG_SEND>3
        MW_LOG_MODULE(module,param); MW_LOG_LN((char *) F("frameModuleParamInf"));
#endif
        frameEnd();
        return _size;
    }

    /**
     * Отправить на сервер фрейм Значение параметра
     * @return Размер отправленного
     */
    size_t SendModuleParamValue(MWOSModuleBase * module, MWOSParam * param, uint16_t index) {
        if (!module || !param) return 0;
        frameStart();
        // 0: Код фрейма: Данные о параметре модуля
        if (!beginFrame(mwos_server_cmd_param_set_value,module->id,param->id, index)) return 0; // 7
        // 7:Значение параметра (длина данных значения зависит от типа параметра)
        MWValue data(param->valueType, module->id, param->id, index);
        size_t len=min(_net->sendPacket.available()-_size-2, (int) param->arrayCount());
        data.setBuffer(frame_buffer+_size,len); // что-бы данные сразу копировались в буфер
        module->getValue(data, param);
        if (data.status.error) clear(_size+1); // выдадим ошибку
        else {
            _size += data.size; // сколько реально загрузили
        }
#if LOG_SEND>3
        MW_LOG_MODULE(module,param,index); MW_LOG_LN((char *) F("frameModuleParamValue"));
#endif
        if (_size>0) module->SetParamChanged(param,index, false); // сбросим флаг отправки
        frameEnd();
        return _size;
    }

    /**
     * Отправить на сервер фрейм все значения параметра
     * @return Размер отправленного
     */
    size_t SendModuleParamAllValues(MWOSModuleBase * module, MWOSParam * param) {
        if (!module || !param) return 0;
        frameStart();
        // 0: Код фрейма: Данные о параметре модуля
        if (!beginFrame(mwos_server_cmd_param_set_param_all_values,module->id,param->id, 0)) return 0; // 7
        // 7:Значение параметра (длина данных значения зависит от типа параметра)
        if (param->IsString()) { // строки запросим целиком
            MWValue data(param->valueType, module->id, param->id, 0xffff);
            size_t len=min(_net->sendPacket.available()-_size-2, (int) param->arrayCount());
            data.setBuffer(frame_buffer+_size,len); // что-бы данные сразу копировались в буфер
            module->getValue(data, param);
            if (data.status.error) clear(_size+1); // выдадим ошибку
            else {
                _size += data.size; // сколько реально загрузили
            }
        } else
        for (int i = 0; i < param->arrayCount(); ++i) { // запрашиваем каждое значение в массиве
            MWValue data(param->valueType, module->id, param->id, i);
            module->getValue(data, param);
            if (data.size>0) {
                write(data.getBytes(),data.size);
            }
        }
#if LOG_SEND>3
        MW_LOG_MODULE(module,param); MW_LOG_LN((char *) F("frameModuleParamAllValues"));
#endif
        if (_size>0) module->SetParamChanged(param,UINT16_MAX, false); // сбросим флаг отправки
        frameEnd();
        return _size;
    }

private:
    /**
     * Начало нового фрейма. Заодно сбрасывает ошибки.
     */
    void frameStart() {
        setBuffer(_net->sendPacket._buffer + _net->sendPacket._size, _net->sendPacket.available());
        // запишем 5 байт начала пакета
        write(BLOCK_DATA);
        writeUInt16(_net->getNextPacketID());
        writeUInt16(0); // пока запишем 0, вместо длины
    }

    /**
     * Конец фрейма.
     */
    void frameEnd() {
        if (_size==0) return;
        // запишем реальную длину фрейма
        uint16_t len=_size-5;
        memcpy(frame_buffer+3,&len,2);
        // 2 служебных байта в конце блока
        write(0); // количество отправок
        write(_net->sendPacket.GetTimeDSec()-127); // максимально время с момента отправки
        if (_size>0) { // если блок записан успешно
            uint16_t pos = _net->sendPacket._size; // указатель на фрейм
            _net->sendPacket._size += _size; // учтем этот пакет
            // если блок большой - разобьем на пакеты поменьше
            if (_net->SetSendMaxPacketSize(pos)==0) clear(_size+1); // ошибка
        }
    }

};



#endif
