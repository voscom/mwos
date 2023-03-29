#ifndef MWOS3_MWOSCONSTS_H
#define MWOS3_MWOSCONSTS_H

// команды от сервера
// коды команд должны находиться в диапазоне:
// 0-15 - команды параметру с данными,
// 16-31 - команды параметру с индексом массива без данных,
// 32-63 - команды параметру без данных,
// 64-127 - команды модулю без данных,
// 128-255 - команды контроллеру без данных
enum MWOSProtocolCommand : int16_t {
    // ошибка передачи текущего блока данных (если не задан буффер приема)
    mwos_server_cmd_param_error_block = -3,
    // конец передачи текущего блока данных (если не задан буффер приема)
    mwos_server_cmd_param_stop_block = -2,
    // начало передачи текущего блока данных (если не задан буффер приема)
    mwos_server_cmd_param_start_block = -1,
    // установить значение параметра с записью по месту хранения (только для значений менее 64бит)
    mwos_server_cmd_param_set_value = 0,
    // запросить значение параметра с индексом
    mwos_server_cmd_param_get_value = 16,
    // записать текущее значение параметра по месту хранения
    mwos_server_cmd_param_save = 17,
    // перезагрузить текущее значение из места хранения
    mwos_server_cmd_param_reload = 18,
    // запросить все значения параметра
    mwos_server_cmd_param_get_param = 32,
    // бит означает, что это пакет данных для подчиненных контроллеров
    mwos_server_cmd_route = 64,
    // запросить полный формат контроллера
    mwos_server_cmd_get_format = 128,
    mwos_server_cmd_handshake = 129
};

// место хранения значений параметров
enum MWOSParamStorage :uint8_t {
    // параметр хранится в EEPROM
    mwos_param_storage_eeprom =0,
    // параметр хранится в RTC (статик-ОЗУ с неограниченным количеством перезаписей)
    mwos_param_storage_rtc=1,
    // параметр нигде не хранится (обычно для вычисляемых параметров и показаний)
    mwos_param_storage_no=63,

};

// К какой группе относится параметр
enum MWOSParamGroup : uint8_t {
    // не классифицированный параметр
    mwos_param_nogroup=0,
    // это параметр управления
    mwos_param_control=1,
    // это датчик
    mwos_param_sensor=2,
    // это настройки
    mwos_param_option=4,
    // это событие для записи в журнал
    mwos_param_event=8,
    // только для чтения (нельзя отправлять команду записи этого параметра)
    mwos_param_readonly=16
};

// типы юнита
enum UnitType : uint8_t {
    PARAM = 0,
    MODULE = 1,
    OS = 2
};

// типы значений
enum ParamValueType : int16_t {
    mwos_param_string = 1,
    mwos_param_int8 = 2,
    mwos_param_uint8 = 3,
    mwos_param_int16 = 4,
    mwos_param_uint16 = 5,
    mwos_param_int32 = 6,
    mwos_param_uint32 = 7,
    mwos_param_int64 = 8,
    mwos_param_uint64 = 9,
    mwos_param_float32 = 10,
    mwos_param_double64 = 11,
    mwos_param_bits1 = 12,
    mwos_param_bits2 = 13,
    mwos_param_bits3 = 14,
    mwos_param_bits4 = 15,
    mwos_param_bits5 = 16,
    mwos_param_bits6 = 17,
    mwos_param_bits7 = 18,
    // массив однотипных значений. Пример задания: {mwos_param_array} * {кол-во элементов} + {mwos_param_int16}
    mwos_param_array = 32,
};

// определим типы переменных
#define VALUE_mwos_param_string string
#define VALUE_mwos_param_int8 int8_t
#define VALUE_mwos_param_int16 int16_t
#define VALUE_mwos_param_int32 int32_t
#define VALUE_mwos_param_int64 int64_t
#define VALUE_mwos_param_uint8 uint8_t
#define VALUE_mwos_param_uint16 uint16_t
#define VALUE_mwos_param_uint32 uint32_t
#define VALUE_mwos_param_uint64 uint64_t
#define VALUE_mwos_param_float32 float
#define VALUE_mwos_param_double64 double
#define VALUE_mwos_param_bits1 bool
#define VALUE_mwos_param_bits2 uint8_t
#define VALUE_mwos_param_bits3 uint8_t
#define VALUE_mwos_param_bits4 uint8_t
#define VALUE_mwos_param_bits5 uint8_t
#define VALUE_mwos_param_bits6 uint8_t
#define VALUE_mwos_param_bits7 uint8_t

#endif //MWOS3_MWOSCONSTS_H
