#ifndef MWOS3_MWNETPRINT_H
#define MWOS3_MWNETPRINT_H

class MWNetPrint : public Print {
public:
    uint8_t * frame_buffer;
    // максимальный размер данных в буфере
    uint16_t _maxSize=0;
    // размер данных в буфере
    uint16_t _size=0;
    // ошибка на данных в буфере
    uint16_t _errorByte=0;

    /**
     * Задать внешний буфер и его размр
     * @param buffer
     * @param maxSize
     */
    void setBuffer(uint8_t * buffer, uint16_t maxSize) {
        frame_buffer=buffer;
        _maxSize=maxSize;
        clear(0);
    }

    /**
     * Очистить принятое. Если задан байт ошибки > 0, то дальнейшее наполнение буфера
     * приостанавливается до clear(0) или нового setBuffer
     * @param errorByte
     */
    void clear(uint16_t errorByte) {
        _size=0;
        if (errorByte>0 && _errorByte==0) {
#if LOG_SEND>0
            MW_LOG_TIME(); MW_LOG(F("netFrame error: ")); MW_LOG_LN(errorByte);
#endif
        }
        _errorByte=errorByte;
    }

    uint16_t writeUInt32(uint32_t v) {
        return write((uint8_t *) &v,4);
    }

    uint16_t writeInt32(int32_t v) {
        return write((uint8_t *) &v,4);
    }

    uint16_t writeInt64(int64_t v) {
        return write((uint8_t *) &v,8);
    }

    uint16_t writeUInt64(uint64_t v) {
        return write((uint8_t *) &v,8);
    }

    uint16_t writeInt16(int16_t v) {
        return write((uint8_t *) &v,2);
    }

    uint16_t writeUInt16(uint16_t v) {
        return write((uint8_t *) &v,2);
    }

    virtual size_t write(uint8_t byte) {
        if (_errorByte>0 || _size+1>=_maxSize) {
            clear(_size+1);
            return 0;
        }
        frame_buffer[_size++]=byte;
        return 1;
    }

    virtual size_t write(uint8_t * buffer, size_t size) {
        if (_errorByte>0 || _size+size>_maxSize) {
            clear(_size+1);
            return 0;
        }
        memcpy(frame_buffer+_size,buffer,size);
        _size+=size;
        return size;
    }

    /***
     * Напечатать строку из PROGMEM
     * @param pmem_str
     * @return
     */
    size_t write_progmem_str( char * progmem_str) {
        size_t size=strlen_P(progmem_str);
        //if (maxSize>0 && maxSize<size) size=maxSize;
        size_t rsize=0;
        for (size_t i = 0; i < size; i++) {
            char ch = pgm_read_byte_near(progmem_str + i);
            rsize+=write(ch);
        }
        rsize+=write((uint8_t) 0); // конец строки
        return rsize;
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


};


#endif //MWOS3_STM32F103_MWOSNETTRANSMITTER_H
