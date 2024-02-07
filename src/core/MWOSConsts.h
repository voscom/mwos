#ifndef MWOS3_MWOSCONSTS_H
#define MWOS3_MWOSCONSTS_H

#ifndef MWOS_MINIMUM_RAM // если не включен режим экономии ОЗУ - стандартные ограничения
#define MWOS_PARAM_UINT uint16_t // какой переменной задается количество параметров (или модулей) uint16_t или uint8_t
#define MWOS_PARAM_INT_PTYPE mwos_param_int16 // тип параметра модуля, для ссылки на номер параметра (или модуля) mwos_param_int16 или mwos_param_int8
#define MWOS_PARAM_UINT_PTYPE mwos_param_int16 // тип параметра модуля, для ссылки на номер параметра (или модуля) mwos_param_uint16 или mwos_param_uint8
#define MWOS_PARAM_INDEX_UINT int16_t // какой переменной задается количество индексов в параметре int16_t или int8_t
#define MWOS_PARAM_INDEX_UINT_PTYPE mwos_param_uint16 // тип параметра модуля, для ссылки на индекс параметра mwos_param_int16 или mwos_param_int8
#define MWOS_PIN_INT int16_t // какой переменной задается порт микроконтроллера int16_t или int8_t
#define MWOS_PIN_INT_PTYPE mwos_param_int16 // тип параметра модуля, для задания порта микроконтроллера mwos_param_int16 или mwos_param_int8
#define mwos_size uint32_t // размер
#else // если включен режим экономии ОЗУ (актуально, для экономии памяти на 8-битных микроконтроллерах)
#define MWOS_PARAM_UINT uint8_t // какой переменной задается количество параметров uint16_t или uint8_t
#define MWOS_PARAM_INT_PTYPE mwos_param_int8 // тип параметра модуля, для ссылки на номер параметра mwos_param_int16 или mwos_param_int8
#define MWOS_PARAM_INDEX_UINT uint8_t // какой переменной задается количество индексов в параметре int16_t или int8_t
#define MWOS_PARAM_INDEX_UINT_PTYPE mwos_param_uint8 // тип параметра модуля, для ссылки на индекс параметра mwos_param_int16 или mwos_param_int8
#define MWOS_PIN_INT int8_t // какой переменной задается порт микроконтроллера int16_t или int8_t
#define MWOS_PIN_INT_PTYPE mwos_param_int8 // тип параметра модуля, для задания порта микроконтроллера mwos_param_int16 или mwos_param_int8
#define mwos_size uint16_t // размер
#endif

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
    mwos_server_cmd_param_set_value = 5,
    // запросить значение параметра с индексом
    mwos_server_cmd_param_get_value = 16,
    // записать текущее значение параметра по месту хранения
    mwos_server_cmd_param_save = 17,
    // перезагрузить текущее значение из места хранения
    mwos_server_cmd_param_reload = 18,
    // запросить все значения параметра
    mwos_server_cmd_param_get_param = 32,
    // установить все значения параметра
    mwos_server_cmd_param_set_param_all_values = 33,
    // данные о параметре модуля
    mwos_server_cmd_param_inf = 34,

    // данные о модуле
    mwos_server_cmd_module_inf = 65,


    // запросить полный формат контроллера
    mwos_server_cmd_get_format = 128,
    mwos_server_cmd_handshake = 129,
    // коды фреймов
    mwos_server_cmd_frame_firmware = 130, // фрейм с данными о прошивке
    mwos_server_cmd_frame_storages = 131, // фрейм с данными о хранилищах
    mwos_server_cmd_frame_json = 133 // фрейм с данными о модулях и параметрах
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

// К какой группе и типу относится параметр
// mwos_param_readonly, mwos_param_secret, mwos_param_option - можно задавать одновременно с другими
// 0-31 - как позывать/задавать параметр
enum MWOSParamGroup : uint8_t {
    // параметр задается числом или строкой
    // зависит от ParamValueType
    // или как перечисление
    // зависит от пользовательского формата значения
    // если не задано mwos_param_option, mwos_param_readonly или mwos_param_pin,
    // то параметры будут показаны в разделе "Параметры управления"
    mwos_param_control  = 0,
    // параметр задается как порт контроллера
    // параметр будет всегда показан в разделе "Настройки портов"
    mwos_param_pin      = 1,
    // параметр задается как время или дата
    // или вместе дата и время
    // зависит от пользовательского формата значения
    mwos_param_time     = 2,
    // аналогично mwos_param_time, но с отсчетом времени вперед
    // каждую секунду значение увеличивается на 1
    mwos_param_realtime = 3,
    // параметр задается как id модуля на этом контроллере
    // может быть показан в виде выпадающего списка модулей
    mwos_param_module_id= 4,
    // параметр задается в градусах или как час`мин`сек`градус
    mwos_param_grad     = 5,

    // это параметр показывать в разделе "Настройки модулей"
    // если не задано mwos_param_pin
    mwos_param_option   = 32,
    // это параметр только для чтения
    // если не задано mwos_param_option или mwos_param_pin,
    // то параметры будут показаны в разделе "Датчики"
    // дистанционное изменение значения этого параметра не сохраняет значение,
    // но может использоваться как команда
    mwos_param_readonly = 64,
    // секретный параметр
    // который никогда не отправляется с контроллера
    mwos_param_secret   = 128,
};

// типы юнита
enum UnitType : uint8_t {
    PARAM = 0,
    MODULE = 1,
    OS = 2
};

// типы модуля
enum ModuleType : uint8_t {
    MODULE_UNDEFINED = 0,
    MODULE_NET = 1,
    MODULE_TIME = 2,
    MODULE_GATE = 3
};

// типы значений
enum ParamValueType : uint8_t {
    mwos_param_value_auto = 0, // тип определяется автоматически (только для переменных виртуальной машины, а не для параметров)
    mwos_param_string = 1, // строка (заканчивается на 0, максимальная длина задается в index)
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
    mwos_param_byte_array = 32, // байтовый массив (длина задается в index)
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
