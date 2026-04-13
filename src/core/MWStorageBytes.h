#ifndef MWOS3_MWOSSTORAGEBYTES_H
#define MWOS3_MWOSSTORAGEBYTES_H


#include "core/MWOSConsts.h"
#include "core/adlib/LibCRC.h"
#include "core/MWOSDebug.h"
#include "core/MWOSUnitName.h"
#include "core/MWStorage.h"

/**
 * Базовый класс работы с битовым хранилищем
 * только для хранилищ с побайтным доступом
 * Необходимо переопределить read и write
 *
 * для esp32 можно использовать память RTC (энергонезависимая). Задавать так:
 * RTC_DATA_ATTR uint8_t _buff[]
 *
 * для stm32 можно использовать BKP-регистры
 *
 */
class MWStorageBytes : public MWStorage {
public:

    MWStorageBytes(char * unit_name) : MWStorage(unit_name) {
        _inited=false;
        _needCommit= false;
        _checkSumSize=2;
    }

    virtual uint8_t read(size_t byteOffset) =0;
    virtual void write(size_t byteOffset, uint8_t v) {}

    virtual bool onInit(mwos_size bitSize, uint16_t crc16modules) {
        MWStorage::onInit(bitSize, crc16modules);
        uint16_t savedCRC = ((uint16_t) read(_offset)) + (((uint16_t) read(_offset+1)) << 8); // прочитаем контрольную сумму версии прошивки
        _inited=(crc16modules == savedCRC);
        MW_LOG_TIME(); MW_LOG_PROGMEM(name); MW_LOG(F(" Storage CRC ")); MW_LOG(savedCRC); MW_LOG('='); MW_LOG(crc16modules); MW_LOG('>'); MW_LOG_LN(_inited);
        return _inited;
    }

    /***
     * Сохранить контрольную сумму хранилища. После этого хранилище считается инициализированным.
     * Не использовать напрямую! Вызывать только через mwos.initStorage
     * @param crc16modules  crc16 из mwos.crc16modules
     */
    virtual void initStorage(uint16_t crc16modules) {
        // сохраним контрольную сумму
        write(_offset,crc16modules & 255);
        write(_offset+1,(crc16modules >> 8) & 255);
        commit();
        MWStorage::initStorage(crc16modules);
    }

    bool setBit(uint8_t bitV, mwos_size bitOffset) {
        mwos_size byteOffset=(bitOffset >> 3)+_offset+_checkSumSize;
        uint8_t bitOffsetInByte=bitOffset & 7;
        uint8_t nowByte=read(byteOffset); // чтение байта
        uint8_t lastByte=nowByte;
        bitWrite(lastByte,bitOffsetInByte,bitV);
#ifdef MWOS_LOG_STORAGE
        MW_LOG_TIME();  MW_LOG(F("bitWrite ")); MW_LOG(byteOffset); MW_LOG(':'); MW_LOG(bitOffsetInByte); MW_LOG('='); MW_LOG_LN(lastByte,2);
#endif
        if (lastByte!=nowByte) {
            write(byteOffset,lastByte); // запишем только изменения
            _needCommit=true;
            return true;
        }
        return false;
    }

    uint8_t getBit(mwos_size bitOffset) {
        mwos_size byteOffset=(bitOffset >> 3)+_offset+_checkSumSize;
        uint8_t bitOffsetInByte=bitOffset & 7;
        uint8_t lastByte=read(byteOffset); // чтение байта
#ifdef MWOS_LOG_STORAGE
        MW_LOG_TIME();  MW_LOG(F("bitRead ")); MW_LOG(byteOffset); MW_LOG(':'); MW_LOG(bitOffsetInByte); MW_LOG('='); MW_LOG_LN(lastByte,2);
#endif
        return bitRead(lastByte,bitOffsetInByte);
    }

    /***
     * Сохранить массив значений в хранилище
     * @param dataArray Массив значений
     * @param bitOffset Смещение (бит) в хранилище
     * @param bitSize   Размер в битах
     * @param maxByteSize   Ограничение на размер в хранилище (в байтах)
     * @return Сколько байт записано
     */
    virtual mwos_size saveValue(uint8_t * dataArray, mwos_size bitOffset, mwos_size bitSize, mwos_size maxByteSize=0) {
        if (bitOffset+bitSize>_bitSize) {
            MW_LOG_TIME(); MW_LOG_PROGMEM(name); MW_LOG(F(" save ERROR: ")); MW_LOG(bitOffset+bitSize); MW_LOG('>'); MW_LOG_LN(_bitSize);
            error(1);
            return 0;
        }
#ifdef MWOS_LOG_STORAGE
        MW_LOG_TIME(); MW_LOG(F("Storage save array ")); MW_LOG(_num); MW_LOG(':'); MW_LOG_PROGMEM(name); MW_LOG('>');  MW_LOG((uint32_t) bitOffset); MW_LOG(':'); MW_LOG_LN(bitSize);
#endif
        bool writeChanged=false;
        uint8_t bitOffsetInByte=bitOffset & 7;
        mwos_size byteSize=1;
        if (bitOffsetInByte==0 && bitSize>7) { // это можно копировать побайтно
            mwos_size byteOffset=(bitOffset >> 3)+_offset+_checkSumSize;
            byteSize=bitSize >> 3;
            if ((bitSize & 7)>0) byteSize++;
            if (maxByteSize>0 && byteSize>maxByteSize) byteSize=maxByteSize;
            for (uint8_t i0 = 0; i0 < byteSize; i0++) {
                uint8_t nowByte=read(byteOffset + i0);
                if (nowByte!=dataArray[i0]) {
                    write(byteOffset + i0, dataArray[i0]);
                    _needCommit=true;
                    writeChanged=true;
                }
            }
        } else { // скопируем побитно
            uint8_t value=dataArray[0];
            for (uint8_t i1 = 0; i1 < bitSize; i1++) {
                if (setBit(value & 1, bitOffset + i1)) {
                    writeChanged=true;
                }
                value = value >> 1;
            }
        }
        if (writeChanged) {
            return byteSize;
        }
        return 0;
    }


    /**
     * Прочитать в буфер значение
     * @param buff      Буфер для чтения
     * @param bitOffset Смещение (бит) в хранилище
     * @param bitSize   Размер в битах
     * @param maxByteSize   Ограничение на размер в хранилище (в байтах)
     * @return Считано байт
     */
    virtual mwos_size loadValue(uint8_t * buff, mwos_size bitOffset, mwos_size bitSize, mwos_size maxByteSize=0) {
        if (bitOffset+bitSize>_bitSize) {
            MW_LOG_TIME(); MW_LOG_PROGMEM(name); MW_LOG(F(" load ERROR: ")); MW_LOG(bitOffset+bitSize); MW_LOG('>'); MW_LOG_LN(_bitSize);
            error(2);
            return 0;
        }
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (bitOffsetInByte==0 && bitSize>7) { // это можно копировать побайтно
            mwos_size byteOffset=(bitOffset >> 3)+_offset+_checkSumSize;
            mwos_size byteSize = bitSize >> 3;
            if ((bitSize & 7)>0) byteSize++;
            for (mwos_size i = 0; i < byteSize; i++) {
                buff[i]=read(byteOffset + i);
            }
            return byteSize;
        } else {
            uint8_t value=0;
            for (int8_t i = bitSize-1; i >= 0; i--) {
                value = value << 1;
                value |= getBit(bitOffset+i);
            }
            buff[0]=value;
        }
        return 1;
    }

    int8_t error(int8_t errorCode) {
        MW_LOG_TIME(); MW_LOG(F("MWOSStorageBytes error ")); MW_LOG_PROGMEM(name); MW_LOG('>');  MW_LOG_LN(errorCode);
        return errorCode;
    }


};


#endif //MWOS3_MWOSSTORAGE_H
