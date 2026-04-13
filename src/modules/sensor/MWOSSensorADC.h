#ifndef MWOS35_MWOSSENSORADC_H
#define MWOS35_MWOSSENSORADC_H

#include "core/MWOSModule.h"
#include "core/MWOSSensorAnalog.h"

#ifndef MWOS_SENSOR_PULL_TIME_MS
// минимальное время включения подтяжки на портах перед замером показаний
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

/**
* Шаблонный класс для аналоговых датчиков АЦП (Версия MWOS3.5)
*
* Наследуется от MWOSSensorAnalog, добавляет параметры портов и настройки АЦП.
* Поддерживает усреднение нескольких замеров для повышения точности.
* Для ESP32 доступна настройка затухания сигнала АЦП.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение датчиков).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT sensorsCount, class MWOSSensorAnalogType = MWOSSensorAnalog<sensorsCount>>
class MWOSSensorADC : public MWOSSensorAnalogType {
public:
    // номера портов для каждого датчика
    MWOS_PIN_INT _pin[sensorsCount];
    // количество замеров подряд для усреднения
    uint8_t _mean = 0;

    // --- Объявление параметров (автоматическая регистрация) ---

    // порты датчиков
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, sensorsCount);
    #ifdef ESP32
    // затухание АЦП (только для ESP32)
    MWOS_PARAM_FF(20, attenuation, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount,
                  "ADC_DB_0;ADC_DB_2_5;ADC_DB_6;ADC_DB_12");
    #endif
    // количество замеров подряд для усреднения до среднего арифметического
    MWOS_PARAM(21, mean, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    MWOSSensorADC() : MWOSSensorAnalogType() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i] = -1;
        }
    }

    MWOSSensorADC(const uint8_t * pins) : MWOSSensorAnalogType() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            uint8_t pin = pgm_read_byte_near(pins + i);
            if (pin == 255) _pin[i] = -1;
            else _pin[i] = pin;
        }
    }

    MWOSSensorADC(MWOS_PIN_INT pin) : MWOSSensorAnalogType() {
        if (pin == 255) _pin[0] = -1;
        else _pin[0] = pin;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                _mean = MWOSModule::loadValueInt(_mean, p_mean, 0);
                for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
                    _pin[i] = MWOSModule::loadValueInt(-1, p_pin, i);
                    #ifdef ESP32
                    // Загрузка настройки затухания для ESP32
                    #endif
                }
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_pin:
                        _pin[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_pin, data.param_index);
                        break;
                    #ifdef ESP32
                    case id_attenuation:
                        // Обработка настройки затухания
                        MWOSModuleBase::SetParamChanged(&p_attenuation, data.param_index);
                        break;
                    #endif
                    case id_mean:
                        _mean = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_mean);
                        break;
                    default:
                        break;
                }
            } break;

            case EVENT_UPDATE: {
                // Тиковая обработка выполняется в базовом классе
            } break;

            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_pin:
                        data.setValueInt(_pin[data.param_index]);
                        break;
                    #ifdef ESP32
                    case id_attenuation:
                        data.setValueInt(0); // Заглушка для затухания
                        break;
                    #endif
                    case id_mean:
                        data.setValueInt(_mean);
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

    virtual void initSensor(int16_t index, bool pullOn) override {
        MWOSSensorAnalogType::initSensor(index, pullOn);

        _pin[index] = MWOSModule::loadValueInt(_pin[index], p_pin, index);
        _mean = MWOSModule::loadValueInt(_mean, p_mean, 0);

        if (_pin[index] < 0) return;

        MW_PIN_MODE mode = MW_PIN_INPUT;
        if (pullOn || !MWOSSensorAnalogType::_pull_off) {
            mode = (MW_PIN_MODE)MWOSSensorAnalogType::_sensor_pull;
        }

        if (!mwos.mode(_pin[index], mode)) {
            _pin[index] = -3;
            MWOSModule::saveValueInt(-3, p_pin, index);
            MWOSModuleBase::SetParamChanged(&p_pin, index);
        }

        #ifdef ESP32
        // Настройка затухания АЦП для ESP32
        // analogSetPinAttenuation(_pin[index], (adc_attenuation_t)attenuation);
        #endif
    }

    /**
    * Опросить аналоговый датчик для получения новых показаний
    */
    virtual int32_t readAnalogValue(int16_t arrayIndex) override {
        if (_mean < 2) {
            return mwos.readValueAnalog(_pin[arrayIndex]);
        }

        double summ = 0;
        for (int i = 0; i < _mean; i++) {
            if (i > 0) delay(1);
            summ += mwos.readValueAnalog(_pin[arrayIndex]);
        }

        summ = round(summ / ((double)_mean));
        return (int32_t)summ;
    }

    /**
    * Опросить цифровой датчик для получения новых показаний
    */
    virtual int32_t readDigitalValue(int16_t arrayIndex) override {
        return mwos.readValueDigital(_pin[arrayIndex]);
    }
};

#endif