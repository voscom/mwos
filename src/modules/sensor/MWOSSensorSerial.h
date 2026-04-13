#ifndef MWOS35_MWOSSENSOR_SERIAL_H
#define MWOS35_MWOSSENSOR_SERIAL_H

#include "core/MWOSModule.h"
#include "core/MWOSSensorAnalog.h"

#ifndef MWOS_SENSOR_PULL_TIME_MS
// минимальное время включения подтяжки на портах перед замером показаний
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

// количество контактов для подключенного устройства (не менее 2х)
#ifndef MWOSSensorStreamPinsCount
#define MWOSSensorStreamPinsCount 2
#endif

// скорость последовательного порта
enum TypeBaudrate : uint8_t {
    // нет скорости
    BR_NO = 0,
    // 1200 бод
    BR_1200 = 1,
    // 2400 бод
    BR_2400 = 2,
    // 4800 бод
    BR_4800 = 3,
    // 9600 бод
    BR_9600 = 4,
    // 14400 бод
    BR_14400 = 5,
    // 19200 бод
    BR_19200 = 6,
    // 28800 бод
    BR_28800 = 7,
    // 38400 бод
    BR_38400 = 8,
    // 57600 бод
    BR_57600 = 9,
    // 115200 бод
    BR_115200 = 10,
};

/**
* Универсальный модуль получения потоковых данных от датчика (Версия MWOS3.5)
*
* Показания датчика считываются с последовательного порта.
* Может получать показания практически с любого потокового датчика (данные текстовые).
* Для каждого параметра датчика ожидается буква, потом число (до другой буквы или конца строки).
* Десятичные точки и запятые игнорируются. Значение параметра - целое число со знаком.
*
* Пример потока от датчика LuminOx:
* O 0300.0 T +25.7 P 0959 % 025.00 e 0004/r/n
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение порта, парсинг данных).
*   EVENT_GET_VALUE - возврат значений через switch.
*
* Пример создания датчика LuminOx:
* const uint8_t sensPinsO2[] PROGMEM = { (uint8_t) PIN_SENSOR_O2_RX, (uint8_t) PIN_SENSOR_O2_TX };
* const uint8_t profileLuminOx[] PROGMEM = { (uint8_t) BR_9600, 'O','T','P','%','e' };
* MWOSSensorSerial<4> sensorSerialO2(&MWOS_SensorO2Serial, (uint8_t *)&profileLuminOx, (uint8_t *)&sensPinsO2);
*/
template<MWOS_PARAM_INDEX_UINT sensorsCount, class MWOSSensorAnalogType=MWOSSensorAnalog<sensorsCount>>
class MWOSSensorSerial : public MWOSSensorAnalogType {
public:
    // сырые значения для каждого параметра
    int32_t raw[sensorsCount + 1];
    // усредненные значения для каждого параметра
    int32_t avalue[sensorsCount + 1];
    // буква для каждого параметра датчика, после которой идет значение
    uint8_t _streamChar[sensorsCount + 1];
    // количество отправленных символов запроса
    uint8_t numSendChar = 0;
    // флаг получения сырого ответа
    bool rawAnswer = false;
    // текущая скорость порта
    TypeBaudrate _baudrate = BR_NO;
    // пины, занимаемые датчиком (RX, TX, ...)
    MWOS_PIN_INT _pin[MWOSSensorStreamPinsCount];
    // указатель на последовательный порт
    Stream *_serial;
    // команда перевода датчика в режим запросов
    String _commandModeASK = "";
    // ответ на запрос
    String _answer = "";

    // --- Объявление параметров (автоматическая регистрация) ---

    // порты, используемые для получения и отправки данных
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN + PARAM_TYPE_READONLY, MWOS_STORAGE_NO, MWOSSensorStreamPinsCount);
    // буква для каждого параметра датчика, после которой идет значение
    MWOS_PARAM(20, streamChar, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount + 1);
    // скорость порта Serial (baudrate). Если 0, то не перенастраивается
    MWOS_PARAM(21, baudrate, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    /**
    * Конструктор модуля
    * @param serial Последовательный порт
    * @param profile Профиль датчика. Первым байтом - скорость, потом - буквы параметров
    * @param pins Пины, занимаемые датчиком (RX, TX, ...)
    */
    MWOSSensorSerial(Stream * serial, const void * profile, const uint8_t * pins = nullptr) : MWOSSensorAnalogType() {
        MWOSModuleBase::setName((char *) F("sensorSerial"));
        _serial = serial;
        if (pins)
        for (MWOS_PARAM_INDEX_UINT i = 0; i < MWOSSensorStreamPinsCount; ++i) {
            _pin[i] = pgm_read_byte_near(pins + i);
            if (_pin[i] == 255) _pin[i] = -1;
        }
        for (uint8_t i = 0; i < sensorsCount; ++i) {
            raw[i] = 0;
            avalue[i] = 0;
        }
        setProfile((const uint8_t *) profile, true);
    }

    /**
    * Установить профиль в PROGMEM для нового датчика
    * @param profile Профиль датчика
    * @param fromProgMem Из PROGMEM, иначе - из ОЗУ
    */
    void setProfile(const uint8_t *profile, bool fromProgMem) {
        TypeBaudrate baudrate;
        if (fromProgMem) baudrate = (TypeBaudrate) pgm_read_byte_near(profile);
        else baudrate = (TypeBaudrate) profile[0];

        if (_baudrate != baudrate) {
            _baudrate = baudrate;
            if (mwos.IsStorageInited(0)) {
                MWOSModule::saveValueInt((uint8_t) _baudrate, p_baudrate, 0);
                MWOSModuleBase::SetParamChanged(&p_baudrate, 0);
            }
            if (_baudrate > 0) setBaudrate();
        }

        for (int8_t i = 0; i <= sensorsCount; i++) {
            uint8_t ch;
            if (fromProgMem) ch = pgm_read_byte_near(profile + i + 1);
            else ch = profile[i + 1];
            if (_streamChar[i] != ch) {
                _streamChar[i] = ch;
                if (mwos.IsStorageInited(0)) {
                    MWOSModule::saveValueInt(ch, p_streamChar, i);
                    MWOSModuleBase::SetParamChanged(&p_streamChar, i);
                }
            }
        }

        _commandModeASK = "";
        int n = 0;
        while (n < 20) {
            uint8_t ch;
            if (fromProgMem) ch = pgm_read_byte_near(profile + n + sensorsCount + 2);
            else ch = profile[n + sensorsCount + 2];
            if (ch == 0) n = 20;
            else _commandModeASK += (char) ch;
            n++;
        }

        MWValue data;
        if (mwos.IsStorageInited(0)) onEvent(EVENT_CHANGE, data);
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                MWOSSensorAnalogType::_sensor_digital = 0;
                MWOSSensorAnalogType::_pull_off = 0;

                TypeBaudrate baudrate = _baudrate;
                _baudrate = (TypeBaudrate) MWOSModule::loadValueInt((uint8_t) _baudrate, p_baudrate, 0);
                if (baudrate != _baudrate && _baudrate > 0) {
                    setBaudrate();
                }

                for (int8_t i = 0; i <= sensorsCount; i++) {
                    _streamChar[i] = MWOSModule::loadValueInt(_streamChar[i], p_streamChar, i);
                }

                indexCmd = -1;
                if (_commandModeASK != "") SendStr(_commandModeASK, true);
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_baudrate:
                        _baudrate = (TypeBaudrate) data.toInt();
                        if (_baudrate > 0) setBaudrate();
                        MWOSModuleBase::SetParamChanged(&p_baudrate);
                        break;
                    case id_streamChar:
                        _streamChar[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_streamChar, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;

            case EVENT_UPDATE: {
                while (_serial->available() > 0) {
                    char ch = _serial->read();
                    if (rawAnswer) {
                        _answer += ch;
                    } else {
                        if (ch > '9' || ch == '%') indexCmd = IndexOfCmd(ch);
                        if (indexCmd >= 0) {
                            if (waitDigit && ch == '-') {
                                minus = true;
                            } else if (ch >= '0' && ch <= '9') {
                                raw[indexCmd] *= 10;
                                if (minus) raw[indexCmd] -= (ch - '0');
                                else raw[indexCmd] += (ch - '0');
                                waitDigit = false;
                            } else if (ch == 13 || ch == 10 || (!waitDigit && ch == ' ')) {
                                if (indexCmd >= sensorsCount) MWOSSensorAnalogType::setError(raw[sensorsCount]);
                                else {
                                    avalue[indexCmd] = raw[indexCmd];
                                }
                                indexCmd = -1;
                            }
                        }
                    }
                }
            } break;

            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_pin:
                        data.setValueInt(_pin[data.param_index]);
                        break;
                    case id_streamChar:
                        data.setValueInt(_streamChar[data.param_index]);
                        break;
                    case id_baudrate:
                        data.setValueInt((uint8_t) _baudrate);
                        break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSSensorAnalogType::onEvent(modeEvent, data);
    }

    /**
    * Опросить аналоговый датчик для получения новых показаний
    */
    virtual int32_t readAnalogValue(int16_t arrayIndex) override {
        if (arrayIndex >= sensorsCount) return 0;
        char ch = _streamChar[arrayIndex];
        if (_commandModeASK != "" && ch != 0) SendChar(ch, true);
        return avalue[arrayIndex];
    }

private:
    // текущий индекс команды
    int8_t indexCmd = -1;
    // флаг ожидания цифры
    bool waitDigit = false;
    // флаг минуса
    bool minus = false;

    /**
    * Найти индекс команды по символу
    */
    int8_t IndexOfCmd(char ch) {
        for (int8_t i = 0; i <= sensorsCount; ++i) {
            if (_streamChar[i] == ch) {
                raw[i] = 0;
                minus = false;
                waitDigit = true;
                return i;
            }
        }
        return -1;
    }

    /**
    * Отправить символ в порт
    */
    size_t SendChar(char ch, bool ln) {
        if (ch == 0) return 0;
        size_t siz = _serial->write(ch);
        if (ln) siz += _serial->print("\r\n");
        rawAnswer = false;
        _answer = "";
        return siz;
    }

    /**
    * Отправить строку в порт
    */
    size_t SendStr(const String &s, bool ln, bool needAnswerString = false) {
        if (s == "") return 0;
        if (needAnswerString) {
            uint32_t start = millis();
            while (_serial->available() > 0 && (millis() - start < 100)) _serial->read();
        }
        size_t siz = _serial->print(s);
        if (ln) siz += _serial->print("\r\n");
        rawAnswer = needAnswerString;
        _answer = "";
        _serial->flush();
        return siz;
    }

    /**
    * Перенастроить скорость порта
    */
    void setBaudrate() {
        if (MWOSSensorAnalogType::_always_off) return;
        uint32_t baudrate = 9600;
        switch (_baudrate) {
            case BR_1200: baudrate = 1200; break;
            case BR_2400: baudrate = 2400; break;
            case BR_4800: baudrate = 4800; break;
            case BR_9600: baudrate = 9600; break;
            case BR_14400: baudrate = 14400; break;
            case BR_19200: baudrate = 19200; break;
            case BR_28800: baudrate = 28800; break;
            case BR_38400: baudrate = 38400; break;
            case BR_57600: baudrate = 57600; break;
            case BR_115200: baudrate = 115200; break;
        }
        // Настройка порта зависит от платформы
    }
};

#endif