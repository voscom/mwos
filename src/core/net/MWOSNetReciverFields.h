#ifndef MWOS3_MWOSNETRESIVER_FIELDS_H
#define MWOS3_MWOSNETRESIVER_FIELDS_H

#include "core/MWOSConsts.h"

#ifndef MWOS_RECIVE_BLOCK_SIZE
#if defined(ESP32) || defined(ESP8266)
#define MWOS_RECIVE_BLOCK_SIZE 8192  // размер буффера для принимаемого блока данных (Если задано и меньше 0xffff - то создает буффер приема)
#else
#define MWOS_RECIVE_BLOCK_SIZE 0xffff  // размер буффера для принимаемого блока данных (Если задано и меньше 0xffff - то создает буффер приема)
#endif
#endif

/**
 * Принятые по связи (в MWOSNetReciver) данные и их аргументы
 */
#pragma pack(push,1)
struct MWOSNetReciverFields {
    uint16_t module_id; // для модуля
    uint16_t param_id; // для параметра модуля
    uint16_t array_index;   // для индекса параметра
    int64_t reciveValue;    // полученное значение (короткое)
    bool valueIsLong= false;   // короткое (reciveValue) или длинное (buffer) значение
#if MWOS_RECIVE_BLOCK_SIZE<0xffff
    uint8_t buffer[MWOS_RECIVE_BLOCK_SIZE]; // полученные данные (длинное)
#else
    uint8_t buffer[1];  // полученный очередной байт данных (для длинных значений без буфферизации)
#endif
    uint16_t offset; // смещение текущего байта в блоке данных (для окончания приема - размер полученных данных)

    /***
     * Вернуть полученную строку
     * @return
     */
    String GetString() {
        return String(buffer,offset);
    }
};
#pragma pack(pop)

#pragma pack(push,1)
struct MWOS_NET_FRAME_64BIT {
    uint8_t cmd;
    uint8_t module_id;
    uint8_t param_id;
    uint8_t index;
    int32_t value;
};
#pragma pack(pop)


#endif //MWOS3_MWOSNETRESIVER_H
