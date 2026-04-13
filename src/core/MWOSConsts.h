#ifndef MWOS3_MWOSCONSTS_H
#define MWOS3_MWOSCONSTS_H

#ifndef mwos_param_color
// цвет в параметре
#define mwos_param_color mwos_param_uint16
#include <cstdint>
#endif

#ifndef mwos_color
// цвет
#define mwos_color uint16_t
#endif

#ifndef MWOS_MINIMUM_RAM // если не включен режим экономии ОЗУ - стандартные ограничения
#define MWOS_PARAM_UINT uint16_t // какой переменной задается количество параметров (или модулей) uint16_t или uint8_t
#define MWOS_PARAM_INT_PTYPE PARAM_INT16 // тип параметра модуля, для ссылки на номер параметра (или модуля)
#define MWOS_PARAM_UINT_PTYPE PARAM_UINT16 // тип параметра модуля, для ссылки на номер параметра (или модуля)
#define MWOS_PARAM_INDEX_UINT int16_t // какой переменной задается количество индексов в параметре int16_t или int8_t
#define MWOS_PARAM_INDEX_UINT_PTYPE PARAM_UINT16 // тип параметра модуля, для ссылки на индекс параметра
#define MWOS_PIN_INT int16_t // какой переменной задается порт микроконтроллера int16_t или int8_t
#define MWOS_PIN_INT_PTYPE PARAM_INT16 // тип параметра модуля, для задания порта микроконтроллера
#define mwos_size uint32_t // размер
#else // если включен режим экономии ОЗУ (актуально, для экономии памяти на 8-битных микроконтроллерах)
#define MWOS_PARAM_UINT uint8_t // какой переменной задается количество параметров uint16_t или uint8_t
#define MWOS_PARAM_INT_PTYPE PARAM_UINT8 // тип параметра модуля, для ссылки на номер параметра
#define MWOS_PARAM_UINT_PTYPE PARAM_UINT8 // тип параметра модуля, для ссылки на номер параметра (или модуля)
#define MWOS_PARAM_INDEX_UINT uint8_t // какой переменной задается количество индексов в параметре int16_t или int8_t
#define MWOS_PARAM_INDEX_UINT_PTYPE PARAM_UINT8 // тип параметра модуля, для ссылки на индекс параметра
#define MWOS_PIN_INT int8_t // какой переменной задается порт микроконтроллера int16_t или int8_t
#define MWOS_PIN_INT_PTYPE PARAM_UINT8 // тип параметра модуля, для задания порта микроконтроллера
#define mwos_size uint16_t // размер
#endif

/********************************** сетевые настройки *******************************/

#ifndef MWOS_RESIVE_TIMEOUT_DSEC
// таймаут приема данных, если ничего не получили за это время - разрываем связь
#define MWOS_RESIVE_TIMEOUT_DSEC 600
#endif


enum MWOSModeEvent : uint8_t {
    // вызывается каждый тик операционной системы
    EVENT_UPDATE,
    // вызывается при первой инициализации модуля
    EVENT_INIT,
    // после изменения и сохранения настроек или пинов
    EVENT_CHANGE,
    // вызывается перед отключением питания. модуль должен аварийно завершить свои действия
    EVENT_POWER_OFF,
    // вызывается после изменения времени (что-бы модули могли это учесть)
    EVENT_TIME_CHANGE,
    // при подключении к серверу
    EVENT_CONNECT,
    // при отключении от сервера
    EVENT_DISCONNECT,
    // по этому событию модули файловой системы загружают значения параметров из файлов конфигурации.
    // если загруженные параметры из неинициализированных хранилищ, то этим параметрам вызывается событие EVENT_SET_VALUE.
    // которое сохраняет значение в хранилище и вызывает событие EVENT_CHANGE модулю этого параметра.
    EVENT_EMPTY_STORAGES,
    // Запрос значения. В data - для кого (модуль, параметр, индекс) и значение по умолчанию.
    EVENT_GET_VALUE,
    // установка значения
    EVENT_SET_VALUE,
    // событие для журнала
    EVENT_LOG_VALUE,
};


// команды от сервера
// коды команд должны находиться в диапазоне:
// 0-15 - команды параметру с данными,
// 16-31 - команды параметру с индексом массива без данных,
// 32-63 - команды параметру без данных,
// 64-127 - команды модулю без данных,
// 128-255 - команды контроллеру без данных
enum MWOSProtocolCommand : int16_t {
    // ошибка передачи текущего блока данных (если не задан буфер приема)
    mwos_server_cmd_param_error_block = -3,
    // конец передачи текущего блока данных (если не задан буфер приема)
    mwos_server_cmd_param_stop_block = -2,
    // начало передачи текущего блока данных (если не задан буфер приема)
    mwos_server_cmd_param_start_block = -1,
    // это фрейм для пересылки подчиненному контроллеру (через гейт)
    mwos_server_frame_for_other_controller = 0,
    // установить значение параметра с записью по месту хранения
    mwos_server_cmd_param_set_value = 5,
    // событие для параметра (генерируется системой для записи в журнал)
    mwos_server_cmd_param_event = 6,
    // задать/снять признак сохранения параметра в журнал (значение 1-установить,0-снять)
    mwos_server_cmd_param_set_log = 7,
    // запросить значение параметра с индексом
    mwos_server_cmd_param_get_value = 16,
    // запросить все значения параметра
    mwos_server_cmd_param_get_param = 32,
    // установить все значения параметра
    mwos_server_cmd_param_set_param_all_values = 33,
    // данные о параметре модуля
    mwos_server_cmd_param_inf = 34,
    // снять признак отправки параметра (подтверждение доставки)
    mwos_server_cmd_param_set_sended = 35,

    // данные о модуле
    mwos_server_cmd_module_list_params_inf = 65,
    mwos_server_cmd_module_values = 66,
    mwos_server_cmd_module_inf = 67,

    // запросить полный формат контроллера
    mwos_server_cmd_get_format = 128,
    mwos_server_cmd_handshake = 129,
    mwos_server_cmd_get_firmware_inf = 130,
    mwos_server_cmd_get_modules = 135,
    mwos_server_cmd_get_format_all_modules_all_params = 136,
    /**
     * С контроллера на сервер, после отправки всех измененных параметров.
     * С сервера на контроллер, после успешного получения данных о прошивке (контроллеру можно отправлять значения)
     */
    mwos_server_cmd_start_session = 137,
    // коды фреймов
    mwos_server_cmd_frame_firmware = 130, // фрейм с данными о прошивке
    mwos_server_cmd_frame_storages = 131, // фрейм с данными о хранилищах
    mwos_server_cmd_frame_json = 133, // фрейм с данными о модулях и параметрах
    mwos_server_cmd_get_all_values = 134, // запрос всех значений всех параметров всех модулей (как правило - один раз после подключения)
    mwos_server_cmd_get_all_controls = 138, // запрос всех управляющих значений всех параметров всех модулей (как правило - один раз после подключения)
    mwos_server_cmd_get_all_sended = 139, // запрос всех отправленных ранее параметров, но на которые не пришло подтверждение
    // коды контроллеров
    mwos_server_cmd_list_cids= 180, // маршрут. количество CID контроллеров (от 180 до 199) для отправки маршрута (только от сервера контроллерам).
    // пользовательские
    mwos_server_cmd_plugin= 200, // коды от 200 и выше - для расширений
};

// место хранения значений параметров (3 bit)
// параметр нигде не хранится (обычно для вычисляемых параметров и показаний)
#define MWOS_STORAGE_NO 7
// параметр сохранять в журнал (может быть перенастроено удаленно). Только, если задан модуль журнала.
#define MWOS_STORAGE_SAVE_TO_LOG 8

#ifndef  MWOS_STORAGE_EEPROM
// номер хранилища в EEPROM
#define MWOS_STORAGE_EEPROM 0
#endif

#ifndef  MWOS_STORAGE_FOR_PIN
#ifdef MWOS_GLOBAL_NO_PIN
// хранилище для портов - не задано
#define MWOS_STORAGE_FOR_PIN MWOS_STORAGE_NO
#else
// хранилище для портов - EEPROM
#define MWOS_STORAGE_FOR_PIN MWOS_STORAGE_EEPROM
#endif
#endif

#ifndef  MWOS_STORAGE_RTC
// номер хранилища в RTC (статик-ОЗУ с неограниченным количеством перезаписей)
#define MWOS_STORAGE_RTC 1
#endif

#ifndef  MWOS_STORAGE_NVS
// номер хранилища в ESP NVS (отдельный раздел на ESP для пар ключ-значение)
#ifdef ESP32
#define MWOS_STORAGE_NVS 2
#else
#define MWOS_STORAGE_NVS MWOS_STORAGE_EEPROM
#endif
#endif

#ifndef  MWOS_STORAGE_LOG_OPTIONS
// номер хранилища для маски признаков записи изменений параметра в журнал
// (сохраняется в начале хранилища битовой маской для каждого параметра всех модулей)
#define MWOS_STORAGE_LOG_OPTIONS MWOS_STORAGE_EEPROM
#endif

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
    PARAM_TYPE_CONTROL  = 0,
    // параметр задается как порт контроллера
    // параметр будет всегда показан в разделе "Настройки портов"
    PARAM_TYPE_PIN      = 1,
    // параметр задается как время или дата
    // или вместе дата и время
    // зависит от пользовательского формата значения
    PARAM_TYPE_TIME     = 2,
    // аналогично mwos_param_time, но с отсчетом времени вперед
    // каждую секунду значение увеличивается на 1
    PARAM_TYPE_REALTIME = 3,
    // параметр задается как id модуля на этом контроллере
    // может быть показан в виде выпадающего списка модулей
    PARAM_TYPE_MODULE_ID= 4,
    // параметр задается в градусах или как час`мин`сек`градус
    PARAM_TYPE_GRAD     = 5,
    // параметр файла. на него можно отправлять файлы. и с него получать файлы.
    PARAM_TYPE_FILE     = 6,

    // это параметр показывать в разделе "Настройки модулей"
    // если не задано mwos_param_pin
    PARAM_TYPE_OPTIONS   = 32,
    // это параметр только для чтения
    // если не задано mwos_param_option или mwos_param_pin,
    // то параметры будут показаны в разделе "Датчики"
    // дистанционное изменение значения этого параметра не сохраняет значение,
    // но может использоваться как команда
    PARAM_TYPE_READONLY = 64,
    // секретный параметр
    // который никогда не отправляется с контроллера
    PARAM_TYPE_SECRET   = 128,
};

// типы юнита (2 bit)
enum UnitType : uint8_t {
    PARAM = 0,
    MODULE = 1,
    OS = 2
};

// типы модуля (6 bit - 0-63)
enum ModuleType : uint8_t {
    MODULE_UNDEFINED = 0,
    MODULE_NET = 1,
    MODULE_TIME = 2,
    MODULE_GATE = 3,
    MODULE_KEYBOARD = 4,
    MODULE_FS = 5, // модули работы с файловой системой
    MODULE_FILE = 6, // модули приема/передачи файлов
    MODULE_SENSOR = 7,
    MODULE_KEY = 8,
    MODULE_ENCODER = 9, // энкодер или счетчик
    MODULE_TABLE = 10,
    MODULE_WIDGET = 12,
    MODULE_DISPLAY = 13,
    MODULE_LOG = 14,
    MODULE_SERIAL = 15,
    MODULE_RADIONET = 16,
};

// типы значений
enum ParamValueType : uint8_t {
    PARAM_AUTO = 0, // тип определяется автоматически (только для переменных виртуальной машины, а не для параметров)
    PARAM_STRING = 1, // строка (заканчивается на 0, максимальная длина задается в index)
    PARAM_INT8 = 2,
    PARAM_UINT8 = 3,
    PARAM_INT16 = 4,
    PARAM_UINT16 = 5,
    PARAM_INT32 = 6,
    PARAM_UINT32 = 7,
    PARAM_INT64 = 8,
    PARAM_UINT64 = 9,
    PARAM_FLOAT32 = 10,
    PARAM_DOUBLE64 = 11,
    PARAM_BITS1 = 12,
    PARAM_BITS2 = 13,
    PARAM_BITS3 = 14,
    PARAM_BITS4 = 15,
    PARAM_BITS5 = 16,
    PARAM_BITS6 = 17,
    PARAM_BITS7 = 18,
};

/**
 * Шаги подключения к серверу MWOS
 */
enum MWOSNetStep : uint8_t {
    // аппаратно отключим сеть
    STEP_NET_CLOSE,
    // аппаратно включим сеть
    STEP_NET_START,
    // дождемся подключения к сети
    STEP_NET_CONNECT,
    // подключимся к серверу
    STEP_SERVER_CONNECT,
    // ждем подключения к серверу и отправки хендшейка
    STEP_SERVER_CONNECTING,
    // дождемся кода контроллера в ответ на хендшейк
    STEP_SERVER_WAIT_ID,
    // успешно подключились к серверу
    STEP_SERVER_CONNECTED,
    // сетевой модуль в режиме AP (входящие соединения для настройки)
    STEP_SERVER_AP,
    // сетевой модуль отключен
    STEP_NET_OFF,
};

/**
 * Причины соединения и разрыва связи
 */
enum MWOSConnectCode : uint8_t {
    DISCONNECT_RECEIVE_TIMEOUT,
    DISCONNECT_NET_TIMEOUT,
    DISCONNECT_CONNECT_TIMEOUT,
    DISCONNECT_CONTROLLER_ID_TIMEOUT,
    DISCONNECT_SERVER_DONT_CONNECT,
    DISCONNECT_NET_DONT_CONNECT,
    DISCONNECT_CONNECT_SERVER_COUNT,
    DISCONNECT_COMMAND,
    DISCONNECT_WIFI_NET_CLOSE,
    DISCONNECT_WIFI_AP_SAVE,
    DISCONNECT_WIFI_NO_CONNECT,
    DISCONNECT_WIFI_USB_OPTIONS,
    DISCONNECT_WIFI_FAST_RECONNECT,
    DISCONNECT_WIFI_SET_ACTIVE,

    CONNECT_COMMAND = 32,

};

/**
 * тип сети
 */
enum MWOSNetType : uint8_t {
    Net_unknow,
    Net_WiFi,
    Net_LAN,
    Net_WiFi_ESP,
};
#define MWOSNetTypeNames "Unknow;WiFi;LAN;ESP WiFi"

// статус связи
enum ConnectStatus: uint8_t {
    CS_netOff,
    CS_netConnecting,
    CS_netConnected,
    CS_serverConnecting,
    CS_serverConnected,
    CS_netAPConnecting,
    CS_netAPConnected,
};

/**
 * статус сети
 */
enum NetStatus: uint8_t {
    // сеть отключена
    NS_netOff,
    // сеть подключена и активна
    NS_netActive,
    // контроллер в состоянии AP (только входящие подключения)
    NS_netAP,
};

enum NetPacketEvent: uint8_t {
    // вызывается после получения блока данных (или распаковки большого блока данных)
    NetPacketReceive,
    NetPacketBeforeSend,
    NetPacketAfterSend,
    NetPacketReceiveError,
};


#endif //MWOS3_MWOSCONSTS_H
