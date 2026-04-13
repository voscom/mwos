#ifndef CAN_BUS_LIB_H
#define CAN_BUS_LIB_H

typedef struct {
    uint32_t identifier;                /**< 11 or 29 bit identifier */
    uint8_t data_length_code;           /**< Data length code */
    uint8_t data[8];    /**< Data bytes (not relevant in RTR frame) */
} CAN_msg_t;

/***
 * Настроить шину CAN
 * @param pinRx Порт RX
 * @param pinTx Порт TX
 * @param speedKBPS Скорость шины Kbit/sec: 50, 100, 125, 250, 500, 1000
 * @param filter_id Фильтр id пакетов (-1 - все пакеты)
 * @return  Если успешно
 */
bool CAN_Init(uint8_t pinRx, uint8_t pinTx, uint16_t speedKBPS=250, int32_t filter_id=-1) {
    return false;
}

/**
 * Получить сообщение из шины CAN
 * @param message   Место для полученного сообщения
 * @return  Получено или нет
 */
bool CAN_Receive(CAN_msg_t * message) {
    return false;
}

/**
 * Отправить сообщение на шину CAN
 * @param message   Сообщение
 * @return  Отправлено или нет
 */
bool CAN_Send(CAN_msg_t * message) {
    return false;
}

/**
 * Отключить CAN
 * @return Если успешно отключено
 */
bool CAN_Uninit() {
    return false;
}

#endif //CAN_BUS_LIB_H
