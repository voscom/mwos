#ifndef MWOS3_MWSTREAMI2C_H
#define MWOS3_MWSTREAMI2C_H

#include "MWStream.h"
#include <Wire.h>

enum I2C_DEVICE_TYPE:uint8_t {
    I2C_8bit=5,
    I2C_16bit=10,

};

/***
 * Универсальное последовательное хранилище для аппаратного i2c
 * Реализована адресация 8 и 16 бит
 * Автоматическое разделение больших блоков до _blockMaxSize
 */
class MWStreamI2C : public MWStream {
protected:
    TwoWire * _i2c; // на каком порту i2c подключено устройство
    int _device_address; // полный адрес устройства i2c (внутренний + заданный ножками аппаратно)
    I2C_DEVICE_TYPE _i2c_type; // разрядность адресации
    size_t _blockMaxSize=1024; // максимальный размер блока чтения/записи в i2c
public:

    bool begin(TwoWire * i2c,int device_address, I2C_DEVICE_TYPE i2c_type=I2C_16bit, size_t blockMaxSize=1024) {
        _i2c=i2c;
        _device_address=device_address;
        _i2c_type=i2c_type;
        _blockMaxSize=blockMaxSize;
        if (inited) return true;

        return MWStream::begin();
    }

    /**
     * Считать 1 байт по адресу addr
     * @return Считанное значение
     */
    virtual int read() {
        StartI2C();
        uint8_t result = Wire.endTransmission();
        if (result>0) return -1;
        Wire.requestFrom(_device_address, 1);
        addr++;
        return Wire.read();
    }

    /**
     * Записать 1 байт по адресу addr
     * @param val   Значение для записи
     * @return Количество записанных байт
     */
    virtual size_t write(uint8_t val) {
        StartI2C();
        Wire.write(val);
        uint8_t result=Wire.endTransmission();
        if (result>0) return 0;
        addr++;
        return 1;
    }

    /**
     * Считать блок по адресу addr
     * @param buffer
     * @param length
     * @return Количество считанных байт
     */
    virtual size_t readBytes(char *buffer, size_t length) {
        size_t count_bytes=0;
        while (length>0) {
            size_t size=length;
            if (size>_blockMaxSize) size=_blockMaxSize;
            StartI2C();
            uint8_t result = Wire.endTransmission();
            if (result>0) return count_bytes;
            Wire.requestFrom(_device_address, size);
            for (uint16_t i=0; i < size; i++) buffer[count_bytes+i] = Wire.read();
            addr+=size;
            count_bytes+=size;
            length-=size;
        }
        return count_bytes;
    }

    /**
     * Записать блок по адресу addr
     * @param buffer
     * @param length
     * @return Количество записанных байт
     */
    virtual size_t write(const uint8_t *buffer, size_t length) {
        size_t count_bytes=0;
        while (length>0) {
            size_t size=length;
            if (size>_blockMaxSize) size=_blockMaxSize;
            StartI2C();
            for (uint16_t i=0; i < size; i++) Wire.write(buffer[count_bytes+i]);
            uint8_t result = Wire.endTransmission();
            if (result>0) return count_bytes;
            addr+=size;
            count_bytes+=size;
            length-=size;
        }
        return count_bytes;
    }

    /***
     * Отправляет адрес на шину i2c
     */
    void StartI2C() {
        Wire.beginTransmission(_device_address);
        if (_i2c_type>=I2C_16bit) Wire.write((uint8_t) (addr >> 8));
        Wire.write((uint8_t) (addr & 0xFF));
    }


};


#endif //MWOS3_MWSTREAMI2C_H
