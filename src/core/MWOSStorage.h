#ifndef MWOS3_MWOSSTORAGE_H
#define MWOS3_MWOSSTORAGE_H
/**
 * Базовый класс работы с битовым хранилищем
 * Необходимо переопределить read и write
 * может побитно сохранять значения параметров в хранилище
 *
 * для esp32 можно использовать память RTC (энергонезависимая). Задавать так:
 * RTC_DATA_ATTR uint8_t buff[]
 *
 * для stm32 можно использовать BKP-регистры
 *
 */


#include "core/MWOSConsts.h"
#include "adlib/LibCRC.h"
#include "MWOSDebug.h"
#include "MWOSUnitName.h"

class MWOSStorage : public MWOSUnitName {
public:
    static uint8_t count; // количество разных хранилищ
    uint16_t const CheckSumSize=2; // размер контрольной суммы хешей перед хранилищем

#pragma pack(push,1)
    mwos_size _offset=0; // смещение хранилища в байтах
    mwos_size _bitSize;
    uint8_t _inited:1; // признак, что контрольная сумма хранилища сохранена
    uint8_t _needCommit:1; // признак, что была запись без комита
    uint8_t _autoCommitBeforeRead:1; // комитить автоматом, перед чтением (можно изменить из потомков)
    uint8_t _cacheWriteBit:1; // необходимо откладывать в кеш запись последнего байта (актуально, когда записывают несколько бит в один байт)
    uint8_t _cachedWriteByte:1; // признак, что есть закешированный байт для записи
    uint8_t _reserved:3;
    uint8_t lastByte=0;
    mwos_size lastAddr=0;
#pragma pack(pop)

    MWOSStorage() : MWOSUnitName() {
        count++;
        _inited=false;
        _needCommit= false;
        _autoCommitBeforeRead=true;
        _cacheWriteBit=true;
        _cachedWriteByte= false;
    }

    virtual uint8_t read(size_t byteOffset) =0;
    virtual void write(size_t byteOffset, uint8_t v) {}

    /**
     * Комит изменений в хранилище
     * Вызывать из потомков вначале переодределения!
     * @param forced    Сохранить в любом случае (иначе, только если были изменения)
     * @return  Было сохранение
     */
    virtual bool commit(bool forced= false) {
        if (!_needCommit && !forced) return false;
        writeCachedByte();
        _needCommit=false;
        MW_LOG_PROGMEM(name); MW_LOG_LN(F(" - commit storage"));
        return true;
    }

    virtual bool onInit(size_t bitSize, uint16_t crc16modules) {
        _bitSize=bitSize;
        MW_LOG(F("onInit storage ")); MW_LOG_PROGMEM(name); MW_LOG('='); MW_LOG_LN((uint32_t) _bitSize);
        // добавим к контрольной сумме модулей размер хранилища
        MW_CRC16 crc16;
        crc16.crc=crc16modules;
        crc16.add((_bitSize >> 8) & 255);
        crc16.add(_bitSize & 255);

        uint16_t savedCRC = read(_offset) + (read(_offset+1) << 8); // прочитаем контрольную сумму версии прошивки
        bool IsInited=(crc16.crc == savedCRC);
        _inited=IsInited;
        MW_LOG(F("Storage CRC ")); MW_LOG(savedCRC); MW_LOG('='); MW_LOG_LN(crc16.crc);
        return IsInited;
    }

    void writeCachedByte() { // записать закешированный байт, если нужно
        if (_cachedWriteByte) {
            write(lastAddr,lastByte);
            _cachedWriteByte=false;
            MW_LOG(F(" writeCachedByte "));
        }
    }

    /***
     * Сохранить контрольную сумму хранилища. После этого хранилище считается инициализированным.
     * Не использовать напрямую! Вызывать только через mwos.initStorage
     * @param crc16modules  crc16 из mwos.crc16modules
     */
    void initStorage(uint16_t crc16modules) {
        // добавим к контрольной сумме модулей размер хранилища
        MW_CRC16 crc16;
        crc16.crc=crc16modules;
        crc16.add((_bitSize >> 8) & 255);
        crc16.add(_bitSize & 255);
        write(_offset,crc16.crc & 255);
        write(_offset+1,(crc16.crc >> 8) & 255);
        _inited=true;
        MW_LOG(F("Storage inited CRC ")); MW_LOG_PROGMEM(name); MW_LOG('>'); MW_LOG_LN(crc16.crc);
    }

    void setBit(uint8_t bitV, mwos_size bitOffset) {
        mwos_size byteOffset=(bitOffset >> 3)+_offset+CheckSumSize;
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (byteOffset!=lastAddr) {
            writeCachedByte();
            lastAddr=byteOffset;
            lastByte=read(lastAddr); // закешируем чтение последнего байта для записи
        }
        bitWrite(lastByte,bitOffsetInByte,bitV);
        MW_LOG_LN(F("bitWrite"));
        if (!_cacheWriteBit) write(lastAddr,lastByte);
        else _cachedWriteByte=true;
    }

    uint8_t getBit(mwos_size bitOffset) {
        mwos_size byteOffset=(bitOffset >> 3)+_offset+CheckSumSize;
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (byteOffset!=lastAddr) {
            writeCachedByte();
            lastAddr=byteOffset;
            lastByte=read(lastAddr); // закешируем чтение последнего байта для чтения
        }
        MW_LOG_LN(F("bitRead"));
        return bitRead(lastByte,bitOffsetInByte);
    }

    /***
     * Сохранить значение в RAM.
     * Не битовые значения крайне желательно выравнивать по байту (кратно 8 бит)
     * @param value     Значение
     * @param bitOffset Смещение в битах
     * @param bitSize   Размер в битах
      */
    void saveValue(int64_t value, size_t bitOffset, uint8_t bitSize) {
        if (bitOffset+bitSize>_bitSize) {
            error(1);
            return;
        }
        MW_LOG(F("Storage save ")); MW_LOG_PROGMEM(name); MW_LOG('>');  MW_LOG((uint32_t) bitOffset); MW_LOG(':'); MW_LOG(bitSize); MW_LOG('=');  MW_LOG_LN((int32_t) value);
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (bitOffsetInByte==0 && bitSize>7) { // это можно копировать побайтно
            writeCachedByte();
            mwos_size byteOffset=(bitOffset >> 3)+_offset+CheckSumSize;
            mwos_size byteSize=bitSize >> 3;
            uint8_t * dataArray=(uint8_t *) &value;
            for (uint8_t i = 0; i < byteSize; ++i) {
                write(byteOffset+i,dataArray[i]);
            }
        } else { // скопируем побитно
            for (uint8_t i = 0; i < bitSize; ++i) {
                setBit(value & 1,bitOffset+i);
                value = value >> 1;
            }
        }
        _needCommit=true;
    }

    /***
     * Вернуть значение из RAM.
     * @param bitOffset Смещение в битах
     * @param bitSize   Размер в битах
     * @return
     */
    int64_t loadValue(mwos_size bitOffset, int8_t bitSize) {
        if (bitOffset+bitSize>_bitSize) {
            error(2);
            return 0;
        }
        int64_t value=0;
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (_autoCommitBeforeRead && _needCommit) commit(); // закомитим изменения перед чтением
        if (bitOffsetInByte==0 && bitSize>7) { // это можно копировать побайтно
            mwos_size byteOffset=(bitOffset >> 3)+_offset+CheckSumSize;
            uint8_t byteSize = bitSize >> 3;
            uint8_t *dataArray = (uint8_t *) &value;
            for (uint8_t i = 0; i < byteSize; ++i) {
                dataArray[i]=read(byteOffset + i);
            }
        } else {
            for (int8_t i = bitSize-1; i >= 0; i--) {
                value = value << 1;
                value |= getBit(bitOffset+i);
            }
        }
        MW_LOG(F("Storage load ")); MW_LOG_PROGMEM(name); MW_LOG('>');  MW_LOG((uint32_t) bitOffset); MW_LOG(':'); MW_LOG(bitSize); MW_LOG('=');  MW_LOG_LN((int32_t) value);
        return value;
    }

    int8_t error(int8_t errorCode) {
        MW_LOG(F("MWOSStorage error ")); MW_LOG_PROGMEM(name); MW_LOG('>');  MW_LOG_LN(errorCode);
        return errorCode;
    }


};
uint8_t MWOSStorage::count=0;


#endif //MWOS3_MWOSSTORAGE_H
