#ifndef CAN_BUS_LIB_H
#define CAN_BUS_LIB_H

#include "driver/twai.h"

#define CAN_msg_t twai_message_t

/***
 * Настроить шину CAN
 * @param pinRx Порт RX
 * @param pinTx Порт TX
 * @param speedKBPS Скорость шины Kbit/sec: 50, 100, 125, 250, 500, 1000
 * @param filter_id Фильтр id пакетов (-1 - все пакеты)
 * @return  Если успешно
 */
bool CAN_Init(uint8_t pinRx, uint8_t pinTx, uint16_t speedKBPS=250, int32_t filter_id=-1) {
    if (pinRx==255 || pinTx==255) return false;
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t) pinTx, (gpio_num_t) pinRx, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config;
    switch (speedKBPS) {
        case 50: t_config = TWAI_TIMING_CONFIG_50KBITS();
        case 100: t_config = TWAI_TIMING_CONFIG_100KBITS();
        case 125: t_config = TWAI_TIMING_CONFIG_125KBITS();
        case 250: t_config = TWAI_TIMING_CONFIG_250KBITS();
        case 500: t_config = TWAI_TIMING_CONFIG_500KBITS();
        case 1000: t_config = TWAI_TIMING_CONFIG_1MBITS();
        default: t_config = TWAI_TIMING_CONFIG_250KBITS();
    }
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if (filter_id>=0) {
        f_config.acceptance_code=filter_id << 21;
        f_config.acceptance_mask=~((0x7FF << 21) | (0x7FF << 3));
        f_config.single_filter= false;
    }
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        MW_LOG_LN(F("CAN Driver installed"));
    } else {
        MW_LOG_LN(F("CAN Failed to install driver"));
        return false;
    }
    if (twai_start() == ESP_OK) {
        MW_LOG_LN(F("CAN Driver started"));
    } else {
        MW_LOG_LN(F("Failed to start CAN driver"));
        return false;
    }
    return true;
}

/**
 * Отключить CAN
 * @return Если успешно отключено
 */
bool CAN_Uninit() {
    if (twai_stop() == ESP_OK) {
        MW_LOG_LN(F("Driver CAN stopped"));
    } else {
        MW_LOG_LN(F("Failed to stop CAN driver"));
        return false;
    }

    if (twai_driver_uninstall() == ESP_OK) {
        MW_LOG_LN(F("Driver CAN uninstalled"));
    } else {
        MW_LOG_LN(F("Failed to uninstall CAN driver"));
        return false;
    }
    return true;
}

/**
 * Получить сообщение из шины CAN
 * @param message   Место для полученного сообщения
 * @return  Получено или нет
 */
bool CAN_Receive(CAN_msg_t * message) {
    if (twai_receive(message, 0) == ESP_OK) {
        if (!(message->flags & TWAI_MSG_FLAG_RTR)) {
            return true;
        }
    } else {
        MW_LOG(F("CAN Failed to receive message"));
        message->data_length_code=0;
    }
    MW_LOG(F("CAN Failed to receive"));
    return false;
}

/**
 * Отправить сообщение на шину CAN
 * @param message   Сообщение
 * @return  Отправлено или нет
 */
bool CAN_Send(CAN_msg_t * message) {
    message->flags = TWAI_MSG_FLAG_NONE;
    if (twai_transmit(message, 0) == ESP_OK) {
        return true;
    } else {
        MW_LOG(F("CAN Failed to queue message for transmission"));
        return false;
    }
}


#endif //CAN_BUS_LIB_H
