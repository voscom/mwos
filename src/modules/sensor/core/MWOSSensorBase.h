#ifndef MWOS35_MWOSSENSORBASE_H
#define MWOS35_MWOSSENSORBASE_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"

#ifndef MWOS_SENSOR_PULL_TIME_MS
// минимальное время включения подтяжки на портах перед замером показаний
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

// базовый класс для бинарных датчиков
// только для наследования из других модулей датчиков
// необходимо в потомках определить параметр портов через MWOS_PARAM
template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorBase : public MWOSModule {
public:
    // структура битовых флагов для каждого датчика
    #pragma pack(push, 1)
    struct EverySensorBits {
        union {
            // общий байт для всех битов настройки
            uint8_t _value = 0;
            struct {
                // текущее бинарное значение датчика
                uint8_t bin_value : 1;
                // бинарное значение было изменено
                uint8_t bit_changed : 1;
                // включен бинарный фильтр
                uint8_t bit_filter_now : 1;
                // включен запрос
                uint8_t bit_req : 1;
                // аналоговое значение было изменено
                uint8_t analog_changed : 1;
                // резерв
                uint8_t bit_reserve : 3;
            };
        };
    };
    #pragma pack(pop)

    // бинарные показания для каждого датчика
    EverySensorBits _value_bin[sensorsCount];
    // таймеры ожидания для каждого датчика
    MWTimeout<uint8_t,15,true> waitTimeout[sensorsCount];
    // флаг ожидания подтяжки
    uint8_t waitPool : 1;
    // инвертировать значение
    uint8_t _invert : 1;
    // выключать подтяжку порта датчика между опросами
    uint8_t _pull_off : 1;
    // всегда отключено (одновременно с _invert - всегда включено)
    uint8_t _always_off : 1;
    // подтяжка (0=Вход;1=На землю;2=На плюс;3=АЦП)
    uint8_t _sensor_pull : 3;
    // резерв
    uint8_t _reserve1 : 1;
    // фильтр переключения бинарного значения [сек/10]
    uint8_t _bin_filter = 0;
    // минимальное время между опросами показаний датчика [сек/10]
    uint16_t _get_timeout = 1;
    // таймер последнего чтения
    MWTimeout<uint16_t,100> lastRead;
    // количество измерений с момента включения
    uint32_t _valueCount = 0;


    // --- Объявление параметров (автоматическая регистрация) ---

    // бинарные показания (0-вкл, 1-выкл)
    MWOS_PARAM(0, valueBin, PARAM_BITS1, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, sensorsCount);
    // количество измерений с момента включения
    MWOS_PARAM(2, valueCount, PARAM_UINT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);
    // фильтр переключения бинарного значения [сек/15]
    MWOS_PARAM(3, binFilter, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // минимальное время между опросами показаний датчика [сек/10]
    MWOS_PARAM(4, getTimeout, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // инвертировать значение
    MWOS_PARAM(5, invert, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // всегда отключено
    MWOS_PARAM(6, alwaysOff, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // подтяжка (0=Вход;1=На землю;2=На плюс;3=АЦП)
    MWOS_PARAM_FF(7, pull, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "Вход;На землю;На плюс;АЦП");
    // выключать подтяжку порта датчика между опросами
    MWOS_PARAM(8, pullOff, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    MWOSSensorBase() : MWOSModule((char *) F("sensor")) {
        moduleType = MODULE_SENSOR;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                _invert = loadValueInt(_invert, p_invert, 0);
                _pull_off = loadValueInt(_pull_off, p_pullOff, 0);
                _always_off = loadValueInt(_always_off, p_alwaysOff, 0);
                _sensor_pull = loadValueInt(_sensor_pull, p_pull, 0);
                _bin_filter = loadValueInt(_bin_filter, p_binFilter, 0);
                _get_timeout = loadValueInt(_get_timeout, p_getTimeout, 0);

                for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; ++index) {
                    initSensor(index, false);
                }

                SetParamChanged(&p_valueBin);
                lastRead.start(0);
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_invert:
                        _invert = data.toInt();
                        SetParamChanged(&p_invert);
                        break;
                    case id_pullOff:
                        _pull_off = data.toInt();
                        SetParamChanged(&p_pullOff);
                        break;
                    case id_alwaysOff:
                        _always_off = data.toInt();
                        SetParamChanged(&p_alwaysOff);
                        break;
                    case id_pull:
                        _sensor_pull = data.toInt();
                        SetParamChanged(&p_pull);
                        break;
                    case id_binFilter:
                        _bin_filter = data.toInt();
                        SetParamChanged(&p_binFilter);
                        break;
                    case id_getTimeout:
                        _get_timeout = data.toInt();
                        SetParamChanged(&p_getTimeout);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                updateSensor();
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_valueBin: {
                        if (_always_off) _value_bin[data.param_index].bin_value = 0;
                        if (_invert) data.setValueInt(!_value_bin[data.param_index].bin_value);
                        else data.setValueInt(_value_bin[data.param_index].bin_value);
                    } break;
                    case id_valueCount:
                        data.setValueInt(_valueCount);
                        break;
                    case id_binFilter:
                        data.setValueInt(_bin_filter);
                        break;
                    case id_getTimeout:
                        data.setValueInt(_get_timeout);
                        break;
                    case id_invert:
                        data.setValueInt(_invert);
                        break;
                    case id_alwaysOff:
                        data.setValueInt(_always_off);
                        break;
                    case id_pull:
                        data.setValueInt(_sensor_pull);
                        break;
                    case id_pullOff:
                        data.setValueInt(_pull_off);
                        break;
                    default:
                        break;
                }
            } break;


            default:
                break;
        }
        MWOSModule::onEvent(modeEvent, data);
    }

    /**
    * Получить бинарное значение датчика
    */
    bool getValueBool(int16_t arrayIndex = 0) {
        MWValue data;
        data.param_id = id_valueBin;
        data.param_index = arrayIndex;
        onEvent(EVENT_GET_VALUE, data);
        return data.toInt() != 0;
    }

    /**
    * Инициализация каждого датчика
    */
    virtual void initSensor(int16_t index, bool pullOn) {
        // переопределяется в потомках
    }

    /**
    * Вызывается каждый тик операционной системы
    */
    void updateSensor() {
        if (_always_off) return;

        if (lastRead.isTimeout()) { // заодно, обновим время таймеров
            if (_pull_off) {
                // необходимо включить подтяжку перед измерением
                if (!waitPool) {
                    for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; ++index) {
                        initSensor(index, true);
                    }
                    lastRead.startMS(MWOS_SENSOR_PULL_TIME_MS);
                    waitPool = true;
                    return;
                } else {
                    waitPool = false;
                }
            }

            for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; index++) {
                readSensor(index);
                _valueCount++;
            }

            if (_pull_off) {
                for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; ++index) {
                    initSensor(index, false);
                }
            }

            lastRead.startMS(_get_timeout*100);
        }
    }

    /**
    * Чтение значения датчика
    */
    void readSensor(uint16_t index) {
        bool newValueBin = readBoolValue(index);

        if (newValueBin != _value_bin[index].bin_value) {
            if (_bin_filter > 0) {
                if (!_value_bin[index].bit_filter_now) {
                    // надо подождать время, заданное в фильтре
                    _value_bin[index].bit_filter_now = 1;
                    waitTimeout[index].start(_bin_filter,false);
                    return;
                } else {
                    if (!waitTimeout[index].isTimeout(0,false)) return;
                    _value_bin[index].bit_filter_now = 0;
                }
            }

            _value_bin[index].bin_value = newValueBin;
            SetParamChanged(&p_valueBin, index, true);
        }
    }

    /**
    * Вызывается через таймаут для всех индексов
    */
    virtual bool readBoolValue(uint16_t arrayIndex) {
        return true;
    }

    /**
    * Поставить датчик на паузу
    */
    void pause() {
        _always_off = true;
        SetParamChanged(&p_alwaysOff);
    }

    /**
    * Снять с паузы
    */
    void resume() {
        _always_off = false;
        saveValueInt(0, p_alwaysOff, 0);
        SetParamChanged(&p_alwaysOff);
        onEvent(EVENT_CHANGE, MWValue());
        _always_off = false;
    }

    /**
    * Датчик на паузе?
    */
    bool isPaused() {
        return _always_off;
    }
};

#endif
