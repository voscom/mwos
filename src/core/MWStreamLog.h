/*
 * Author: vladimir9@bk.ru
 *
 * Кольцевой последовательный буфер (стим) в ОЗУ.
 * Имеет отдельные указатели для чтения/записи.
 * Может содержать несколько указателей чтения (все кроме первого доступны только для функции readTo).
 * При переполнении записи - удаляет старые данные, но можно переопределить функцию clear.
 *
 */

#ifndef MWStreamRAM_H_
#define MWStreamRAM_H_

#include <Arduino.h>
#include <Stream.h>
#include "adlib/LibCRC.h"

class MWStreamLog : public Stream {
public:

    uint32_t total; // общий размер буфера
    uint32_t count=0; // количество сохраненных блоков
    uint32_t replaced=0; // количество перезаписанных блоков (которые не успели прочитать и удалить снаружи)
    uint8_t * bufferRAM;
    uint32_t offsetWrite=0;
    uint32_t offsetRead=0;

    /**
     * Создать кольцевой последовательный буфер для записи в журнал
     * @param bufferSize	Размер буфера
     */
    MWStreamLog(uint16_t bufferSize) : Stream() {
		total=bufferSize;
		bufferRAM=(uint8_t *)malloc(total); // new uint8_t(total);
	};
	virtual ~MWStreamLog() {
		free(bufferRAM);
	};

    virtual int read() {
        if (available()<1) return -1;
        int res=bufferRAM[offsetRead++];
        if (offsetRead>=total) offsetRead=0;
        return res;
    };

    virtual size_t write(uint8_t b) {
        bufferRAM[offsetWrite++]=b;
        if (offsetWrite>=total) offsetWrite=0;
        return 1;
    }

    virtual size_t write(const uint8_t *buffer, size_t size) {
        for (size_t i = 0; i < size; ++i) write(buffer[i]);
        return size;
    };
    
    virtual void flush() {

    };
    virtual int peek() { return 0; };

    /**
     * Размер непрочитанных данных в буфере
     * @return
     */
    virtual int available() {
        if (offsetWrite==offsetRead) return 0;
        if (offsetWrite>offsetRead) return offsetWrite-offsetRead;
        return total+offsetWrite-offsetRead;
    };

    /**
     * Размер свободного места в буфере, пригодных для записи новых данных.
     * @return
     */
    virtual int availableForWrite() { return total-available(); }

    /**
     * Получить байт первого блока в буфере
     * @param offset    Смещение от начала блока
     * @return  Байт данных блока
     */
    uint8_t getBlockByte(uint32_t offset) {
        offset+=offsetRead;
        if (offset>=total) offset-=total;
        return bufferRAM[offset];
    }

    /**
     * Целое значение без изменения смещения
     * @param offset   Смещение от первого непрочитанного байта
     * @param bytes    Байт в значении (читает указанное количество байт)
     * @return  Значение
     */
    int64_t GetValue(uint32_t offset, uint8_t bytes) {
        int64_t res=0;
        if (available()>offset+bytes) {
            uint32_t offsetR2=offsetRead+offset;
            for (int i = 0; i < bytes; i++) {
                int16_t v= getBlockByte(offsetRead+i);
                if (v<0) return 0;
                res = res << 8;
                res |= v & 0xff;
            }
        }
        return res;
    }

    /**
    * Целое значение UInt32 без изменения смещения
     * @param offset    Смещение от первого не прочитанного байта
     * @return
     */
    uint32_t GetValueUInt32(uint32_t offset) {
        return GetValue(offset,4);
    }

    /**
    * Целое значение UInt16 без изменения смещения
    * @return
    */
    uint16_t GetValueUInt16(uint32_t offset) {
        return GetValue(offset,2);
    }

    /**
     * Получить размер первого блока без изменения смещения
     * @return  Размер первого блока в буфере
     */
    size_t getBlockSize() {
        if (available()==0) return 0;
        return GetValueUInt16(0);
    }

    /**
     * Удалить первый блок в буфере.
     * Обычно, когда блок передан дальше, сохранен или нет места для записи нового блока.
     * @return  Успешно или нет
     */
    bool clearFirstBlock() {
        if (count>0) {
            count--;
            if (count==0) {
                offsetWrite=0;
                offsetRead=0;
            } else {
                offsetRead += getBlockSize() + 2;
                if (offsetRead>=total) offsetRead-=total;
            }
            return true;
        }
        return false;
    }

    /**
     * Начать запись нового блока. Далее необходимо записать данные указанного размера.
     * @param blockSize Размер блока
     * @return  Со
     */
    size_t beginBlock(size_t blockSize) {
        while (availableForWrite()<blockSize+2 && availableForWrite()<total) {
            if (clearFirstBlock()) replaced++; // освободим место, если не лезет и посчитаем перезаписанные события
            else return 0; // для этого события не хватает всего буфера
        }
        write((uint16_t) blockSize);
        count++;
        return 2;
    }

    void clear() {
        offsetWrite=0;
        offsetRead=0;
        count=0;
        replaced=0;
    }

};

#endif /* MWOSLog_H_ */
