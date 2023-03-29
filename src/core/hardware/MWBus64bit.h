#ifndef MWOS3_MWBUS_H
#define MWOS3_MWBUS_H

/***
 * Работа с шиной
 * может работать с сообщениями по 64bit
 * а так же в режиме Stream (с буффером 8 байт)
 */
class MWBus64bit : public Stream {
public:

    /**
     * Создать ненастроенное подключение к шине
     */
    MWBus64bit() : Stream()  {
    }

    /***
     * Прочитать блок данных
     * @param buff  Адрес буффера для чтения (8 байт)
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


};


#endif //MWOS3_MWBUS_CAN_H
