//
// Created by vladimir9@bk.ru
//
#ifndef MWOS3_MWSTREAM_H
#define MWOS3_MWSTREAM_H

#include <Arduino.h>
#include <Stream.h>

/***
 * Базовый класс
 * Последовательное хранилище типа Stream
 * Может содержать адресацию
 *
 * Необходимо переопределить виртуальные методы!
 * минимум эти: read, write
 *
 */
class MWStream : public Stream {
public:

    /***
     * Ардес для операций чтения/записи
     */
    int addr=0;

    /***
     * Признак, что хранилище ранее инициализировано
     */
    bool inited=false;

    virtual bool begin() {
        inited=true;
        return inited;
    }

    virtual bool end() {
        inited=false;
        return inited;
    }

    virtual size_t length() {
        return 0;
    }

    virtual bool commit() {
        return true;
    }

    template<typename T>
    T &get(int address, T &t) {
        addr=address;
        return readBytes((uint8_t*) &t, sizeof(T));
    }

    template<typename T>
    const T &put(int address, const T &t) {
        addr=address;
        return write((uint8_t*) &t, sizeof(T));
    }

    size_t readBuffer(uint16_t address, uint8_t * buffer, uint16_t size) {
        addr=address;
        return readBytes(buffer, size);
    }

    size_t writeBuffer(uint16_t address, uint8_t * buffer, uint16_t size) {
        addr=address;
        return write(buffer, size);
    }

    uint8_t readByte(uint16_t address) {
        addr=address;
        return read();
    }

    uint16_t readWord(uint16_t address) {
        uint16_t val;
        readBuffer(address,(uint8_t *) &val,sizeof(val));
        return val;
    }

    uint32_t readDWord(uint16_t address) {
        uint32_t val;
        readBuffer(address,(uint8_t *) &val,sizeof(val));
        return val;
    }

    uint8_t writeByte(uint16_t address, uint8_t val) {
        addr=address;
        return write(val);
    }

    size_t writeWord(uint16_t address, uint16_t val) {
        return writeBuffer(address,(uint8_t *) &val,sizeof(val));
    }

    size_t writeDWord(uint16_t address, uint32_t val) {
        return writeBuffer(address,(uint8_t *) &val,sizeof(val));
    }

};


#endif //MWOS3_MWSTREAM_H
