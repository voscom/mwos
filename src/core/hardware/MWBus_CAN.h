#ifndef MWOS3_MWBUS_CAN_H
#define MWOS3_MWBUS_CAN_H

#include "MWBus64bit.h"
#if defined(ESP32)
#include "platforms/canbus_esp32.h"

#elif defined(STM32F103xB)
#include "platforms/canbus_stm32f103xB.h"
#else
#include "platforms/canbus_empty.h"
#endif

#define MWOS_SEND_BUFFER_USE 1 // признак, что надо использовать кольцевой буффер отправки

/***
 * Работа с шиной CAN
 * может работать с CAN-сообщениями
 * а так же в режиме Stream (с буффером 8 байт)
 * Можно импользовать с MWOSNetReciver при включенном кольцевом буффере отправки MWOS_SEND_BUFFER_USE
 */

class MWBus_CAN : public MWBus64bit {
public:

    /**
     * Последнее прочитанное сообщение
     */
    CAN_msg_t mess;

    /**
     * Сетевой id подключения
     */
    int32_t id=0;

    /**
     * Шина CANBUS успешно настроена
     */
    bool IsInited=false;

    /**
     * Создать ненастроенное подключение CANBUS
     */
    MWBus_CAN() : MWBus64bit()  {
    }

    /**
     * Создать настроенное подключение CANBUS
     * @param pinRx     Порт CAN RX (CRX)
     * @param pinTx     Порт CAN TX (CTX)
     * @param speedKBPS Скорость [килобит] 50, 100, 125, 250, 500, 1000
     * @param message_id Фильтрация сообщений (-1 - все сообщения, или id сообщения). Сообщения с id=0 пропускает всегда!
     */
    MWBus_CAN(uint8_t pinRx, uint8_t pinTx, uint16_t speedKBPS=250, int32_t message_id=-1): MWBus_CAN() {
        Init(pinRx,pinTx,speedKBPS,message_id);
    }

    /**
     * Перенастроить CANBUS
     * @param pinRx     Порт CAN RX (CRX)
     * @param pinTx     Порт CAN TX (CTX)
     * @param speedKBPS Скорость [килобит] 50, 100, 125, 250, 500, 1000
     * @param message_id Фильтрация сообщений (-1 - все сообщения, или id сообщения). Сообщения с id=0 пропускает всегда!
     */
    void Init(uint8_t pinRx, uint8_t pinTx, uint16_t speedKBPS=250, int32_t message_id=-1) {
        if (IsInited) CAN_Uninit();
        if (message_id>0) id=message_id;
        else id=0;
        IsInited=CAN_Init(pinRx,pinTx,speedKBPS,message_id);
        mess.data_length_code=0;
        messSend.data_length_code=0;
        messSend.identifier=id;
    }

    /***
     * Прочитать сообщение из буффера принятых сообщений в поле mess
     * @return  Если принято
     */
    bool readMessage() {
        bool res=CAN_Receive(&mess);
        if (res) readOffset=0;
        return res;
    }

    /**
     * Прочитать сообщение из буффера принятых сообщений
     * @param message   Структура для чтения сообщения
     * @return  Если успешно
     */
    bool readMessage(CAN_msg_t * message) {
        bool res=CAN_Receive(message);
        if (res) readOffset=0;
        return res;
    }

    /***
     * Прочитать блок данных
     * @param buff  Адрес буффера для чтения (8 байт)
     * @return  Количество считанных байт (0-нет)
     */
    virtual uint8_t readBlock(uint8_t * buff) {
        if (readMessage()) {
            memcpy( &buff, &mess.data, mess.data_length_code); // скопируем данные в сообщение
            return mess.data_length_code;
        }
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
        CAN_msg_t message;
        if (frame_id>=-1) message.identifier = frame_id;
        else message.identifier = id;
        message.data_length_code = size;
        memcpy(&message.data, buff, size); // скопируем данные в сообщение
        return sendMessage(&message);
    }

    /***
     * Отправить приготовленное ранее сообщение
     * @param message   Сообщение
     * @return  Если сообщение успешно отправлено
     */
    bool sendMessage(CAN_msg_t * message) {
        return CAN_Send(message);
    }

    /***
     * Отправить приготовленное ранее сообщение в поле messSend
     * @return  Если сообщение успешно отправлено
     */
    bool sendMessage() {
        bool res=sendMessage(&messSend);
        if (res) messSend.data_length_code=0;
        return res;
    }

    /*** методы Stream ****/

    virtual int available() {
        if (mess.data_length_code<=readOffset) {
            readMessage();
        }
        return mess.data_length_code-readOffset;
    }

    virtual int read() {
        return mess.data[readOffset++];
    }

    virtual int peek() {
        return 0;
    }

    virtual int availableForWrite() {
        return 8-messSend.data_length_code;
    }

    virtual size_t write(uint8_t ch) {
        if (messSend.data_length_code>7) {
            if (!sendMessage()) return 0;
        }
        messSend.data[messSend.data_length_code++]=ch;
        if (messSend.data_length_code>7) {
            flush();
        }
        return 1;
    }

    virtual void flush() {
        messSend.identifier=id;
        sendMessage();
    }

private:
    /**
     * Последнее отправляемое сообщение
     */
    CAN_msg_t messSend;
    uint8_t readOffset=0; // смещение чтения внутри сообщения
};


#endif //MWOS3_MWBUS_CAN_H
