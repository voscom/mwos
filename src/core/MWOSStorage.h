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

class MWOSStorage {
public:
    static uint8_t count; // количество разных хранилищ
    uint16_t const CheckSumSize=2; // размер контрольной суммы хешей перед хранилищем

#pragma pack(push,1)
    size_t _offset=0; // смещение хранилища
    size_t _bitSize;
#pragma pack(pop)

    MWOSStorage() {
        count++;
    }

    virtual uint8_t read(size_t byteOffset) =0;
    virtual void commit() {}
    virtual void write(size_t byteOffset, uint8_t v) {}

    virtual bool onInit(size_t offset, size_t bitSize) {
        _bitSize=bitSize;
        _offset=offset;
        // посчитаем контрольную сумму версии прошивки
        MW_CRC16 crc16;
        crc16.start();
        char *verInRAM = (char *) git_hash_proj; // контрольная сумма хеша проекта
        for (uint8_t i = 0; i < strlen_P(verInRAM); ++i) {
            char ch = pgm_read_byte_near(verInRAM + i);
            crc16.add(ch);
        }
        verInRAM = (char *) git_hash_mwos; // добавим контрольную сумму хеша MWOS
        for (uint8_t i = 0; i < strlen_P(verInRAM); ++i) {
            char ch = pgm_read_byte_near(verInRAM + i);
            crc16.add(ch);
        }
        uint16_t savedCRC = read(_offset) + (read(_offset+1) << 8); // прочитаем контрольную сумму версии прошивки
        bool IsInited=(crc16.crc == savedCRC);
        MW_LOG(F("Storage CRC: ")); MW_LOG(savedCRC); MW_LOG('='); MW_LOG_LN(crc16.crc);
        if (!IsInited) { // сохраним контрольную сумму версии прошивки Storage CRC: 43265=43304
            write(_offset,crc16.crc & 255);
            write(_offset+1,crc16.crc >> 8);
        }
        return IsInited;
    }

    void setBit(uint8_t bitV, size_t bitOffset) {
        size_t byteOffset=(bitOffset >> 3)+_offset+CheckSumSize;
        uint8_t bitOffsetInByte=bitOffset & 7;
        uint8_t byteV=read(byteOffset);
        bitClear(byteV,bitOffsetInByte);
        if (bitV>0) bitSet(byteV,bitOffsetInByte);
        write(byteOffset,byteV);
    }

    uint8_t getBit(size_t bitOffset) {
        size_t byteOffset=(bitOffset >> 3)+_offset+CheckSumSize;
        uint8_t bitOffsetInByte=bitOffset & 7;
        return bitRead(read(byteOffset),bitOffsetInByte);
    }

    /***
     * Сохранить значение в RAM.
     * Не битовые значения крайне желательно выравнивать по байту (кратно 8 бит)
     * @param value     Значение
     * @param bitOffset Смещение в битах
     * @param bitSize   Размер в битах
      */
    void saveValue(int64_t value, size_t bitOffset, uint8_t bitSize, bool needCommit) {
        if (bitOffset+bitSize>_bitSize) {
            error(1);
            return;
        }
        MW_LOG(F("Storage save: ")); MW_LOG(bitOffset); MW_LOG(':'); MW_LOG(bitSize); MW_LOG('=');  MW_LOG_LN((int32_t) value);
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (bitOffsetInByte==0 && bitSize>7) { // это можно копировать побайтно
            size_t byteOffset=bitOffset >> 3;
            size_t byteSize=bitSize >> 3;
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
        if (needCommit) commit();
    }

    /***
     * Вернуть значение из RAM.
     * @param bitOffset Смещение в битах
     * @param bitSize   Размер в битах
     * @return
     */
    int64_t loadValue(size_t bitOffset, int8_t bitSize) {
        if (bitOffset+bitSize>_bitSize) {
            error(1);
            return 0;
        }
        int64_t value=0;
        MW_LOG(F("Storage load: ")); MW_LOG(bitOffset); MW_LOG(':'); MW_LOG(bitSize); MW_LOG('=');  MW_LOG_LN((int32_t) value);
        uint8_t bitOffsetInByte=bitOffset & 7;
        if (bitOffsetInByte==0 && bitSize>7) { // это можно копировать побайтно
            size_t byteOffset = bitOffset >> 3;
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
        return value;
    }

    int8_t error(int8_t errorCode) {
        MW_LOG(F("MWOSStorage error: ")); MW_LOG_LN(errorCode);
        return errorCode;
    }


};
uint8_t MWOSStorage::count=0;


#endif //MWOS3_MWOSSTORAGE_H
