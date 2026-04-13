#ifndef MWOS35_MWOSSENSORBIN_H
#define MWOS35_MWOSSENSORBIN_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include "core/MWOSSensorBase.h"

#ifndef MWOS_SENSOR_PULL_TIME_MS
// минимальное время включения подтяжки на портах перед замером показаний
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

#ifndef MWOS_SENSOR_BIN_ADC
#ifndef MWOS_MINIMUM_RAM
// включена поддержка пинов АЦП [вкл, если не включен режим экономии ОЗУ]
#define MWOS_SENSOR_BIN_ADC 1
#else
// выключена поддержка пинов АЦП
#define MWOS_SENSOR_BIN_ADC 0
#endif
#endif

/**
* Шаблонный класс для бинарных датчиков с поддержкой АЦП
*
* Наследуется от MWOSSensorBase, добавляет параметры портов и порогов АЦП.
* Если adc_from > 0 и пин с АЦП, то бинарной единицей считается аналоговое значение от этого порога.
* Иначе считывается цифровое состояние LOW/HIGH.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение датчиков).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorBin : public MWOSSensorBase<sensorsCount> {
public:
    #pragma pack(push,1)
    // номера портов для каждого датчика
    MWOS_PIN_INT _pin[sensorsCount];
    #if (MWOS_SENSOR_BIN_ADC==1)
    // порог АЦП для каждого датчика: если > 0, то бинарной единицей считается аналоговое значение от этого порога
    uint16_t _adc_from[sensorsCount];
    #endif
    #pragma pack(pop)

    // --- Объявление параметров (автоматическая регистрация) ---

    // порты датчиков
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, sensorsCount);
    #if (MWOS_SENSOR_BIN_ADC==1)
    // порог АЦП: если > 0, то бинарной единицей считается аналоговое значение от этого порога
    MWOS_PARAM(9, adc_from, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);
    #endif

    MWOSSensorBin() : MWOSSensorBase<sensorsCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i] = -1;
            #if (MWOS_SENSOR_BIN_ADC==1)
            _adc_from[i] = 0;
            #endif
        }
    }

    MWOSSensorBin(const uint8_t * pins) : MWOSSensorBin<sensorsCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i] = pgm_read_byte_near(pins + i);
            if (_pin[i] == 255) _pin[i] = -1;
        }
    }

    MWOSSensorBin(MWOS_PIN_INT pin) : MWOSSensorBin<sensorsCount>() {
        _pin[0] = pin;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
                    _pin[i] = MWOSModule::loadValueInt(-1, p_pin, i);
                    #if (MWOS_SENSOR_BIN_ADC==1)
                    _adc_from[i] = MWOSModule::loadValueInt(0, p_adc_from, i);
                    #endif
                }
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_pin:
                        _pin[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_pin, data.param_index);
                        break;
                    #if (MWOS_SENSOR_BIN_ADC==1)
                    case id_adc_from:
                        _adc_from[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_adc_from, data.param_index);
                        break;
                    #endif
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
                    #if (MWOS_SENSOR_BIN_ADC==1)
                    case id_adc_from:
                        data.setValueInt(_adc_from[data.param_index]);
                        break;
                    #endif
                    default:
                        break;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSSensorBase<sensorsCount>::onEvent(modeEvent, data);
    }

    virtual void initSensor(int16_t index, bool pullOn) {
        MWOSSensorBase<sensorsCount>::initSensor(index, pullOn);

        _pin[index] = MWOSModule::loadValueInt(_pin[index], p_pin, index);
        if (_pin[index] < 0) return;

        #if (MWOS_SENSOR_BIN_ADC==1)
        _adc_from[index] = MWOSModule::loadValueInt(_adc_from[index], p_adc_from, index);
        #endif

        MW_PIN_MODE mode = MW_PIN_INPUT;
        if (pullOn || !MWOSSensorBase<sensorsCount>::_pull_off) {
            mode = (MW_PIN_MODE)MWOSSensorBase<sensorsCount>::_sensor_pull;
        }

        if (!mwos.mode(_pin[index], mode)) {
            _pin[index] = -3;
            MWOSModule::saveValueInt(-3, p_pin, index);
            MWOSModuleBase::SetParamChanged(&p_pin, index);
            #if (MWOS_SENSOR_BIN_ADC==1)
            _adc_from[index] = 0;
            MWOSModule::saveValueInt(0, p_adc_from, index);
            MWOSModuleBase::SetParamChanged(&p_adc_from, index);
            #endif
        }
    }

    virtual bool readBoolValue(uint16_t arrayIndex) {
        if (_pin[arrayIndex] < 0) return false;

        bool res = false;
        MWOSPins * pin = mwos.getPin(_pin[arrayIndex]);
        if (pin != NULL) {
            #if (MWOS_SENSOR_BIN_ADC==1)
            if (_adc_from[arrayIndex] > 0) {
                res = pin->readADC() >= _adc_from[arrayIndex];
            } else
            #endif
            {
                res = pin->read();
            }
        }
        return res;
    }
};

#endif