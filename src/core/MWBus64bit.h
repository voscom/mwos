#ifndef MWOS3_MWBUS64b_H
#define MWOS3_MWBUS64b_H

/***
 * Работа с шиной
 * может работать с сообщениями по 64bit
 * а так же в режиме Stream (с буфером 8 байт)
 *
 * Можно использовать с MWOSNetReceiver при включенном кольцевом буфере отправки MWOS_SEND_BUFFER_USE
 */
class MWBus64bit : public Stream {
public:
    uint8_t buffRead[8]; // буфер для чтения
    uint8_t buffWrite[8]; // буфер для отправки

    uint8_t readSize=0; // размер прочитанного сообщения
    uint8_t writeSize=0; // размер отправляемого сообщения

    uint8_t readOffset=0; // смещение чтения внутри сообщения

    /**
     * Создать ненастроенное подключение к шине
     */
    MWBus64bit() : Stream()  {
    }

    /***
     * Прочитать блок данных
     * @param buff  Адрес буфера для чтения (8 байт)
     * @return  Количество считанных байт (0-нет)
     */
    virtual uint8_t readBlock(uint8_t * buff) {
        return 0;
    }

    /***
     * Отправить блок данных сообщением CAN
     * @param buff Блок данных (до 8 байт)
     * @param size  Размер блока данных (1-8 байт)
     * @param frame_id  id-сообщения для отправки данных (если не указано, то использует поле id)
     * @return  Если сообщение успешно отправлено
     */
    virtual bool sendBlock(uint8_t * buff, uint8_t size, int32_t frame_id=-1) {
        return 0;
    }

    /*** методы Stream ****/

    virtual int available() {
        if (readSize<=readOffset) {
            readSize=readBlock((uint8_t * ) &buffRead);
            readOffset=0;
        }
        return readSize-readOffset;
    }

    virtual int read() {
        return buffRead[readOffset++];
    }

    virtual int peek() {
        return 0;
    }

    virtual int availableForWrite() {
        return 8-writeSize;
    }

    virtual size_t write(uint8_t ch) {
        if (writeSize>7) return -1;
        buffWrite[writeSize++]=ch;
        if (writeSize>7) {
            flush();
        }
        return 1;
    }

    virtual void flush() {
        if (writeSize>0) {
            sendBlock((uint8_t * ) &buffWrite,writeSize);
            writeSize=0;
        }
    }



};


#endif //MWOS3_MWBUS64b_H
