#ifndef MWOS35_MWOSSENSORANALOG_H
#define MWOS35_MWOSSENSORANALOG_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include "MWOSSensorBase.h"

// номер параметра с аналоговым значением датчика
#define MWOS_SENSOR_ANALOG_PARAM_ID 9

/**
* Базовый модуль для аналоговых датчиков (Версия MWOS3.5)
*
* Только для наследования из других модулей датчиков!
* Необходимо в потомках определить параметр портов через MWOS_PARAM.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение датчиков).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorAnalog : public MWOSSensorBase<sensorsCount> {
public:
    #pragma pack(push,1)
    // последние аналоговые показания
    int32_t _value[sensorsCount];
    // от какого значения - считать value_bin как бинарная единица
    int32_t _fromValue;
    // до какого значения - считать value_bin как бинарная единица
    int32_t _toValue;
    // аналоговые показания, только если 0. Если 1 - бинарные (при =1 работает полностью аналогично датчику MWOSSensorBin)
    uint8_t _sensor_digital : 1;
    // 0-отправлять только при изменении бинарного значения
    // 1-отправлять при изменении аналогового значения (с учетом фильтров)
    uint8_t _analog_send : 1;
    // вычислять для датчика среднее арифметическое с предыдущим значением
    uint8_t _average : 1;
    // резерв битов
    int8_t _reserveBits : 5;
    // код ошибки для последней операции чтения показаний датчика
    int8_t _errorCode = 0;
    // игнорирует аналоговые изменения датчика меньше этого значения
    uint16_t _analog_filter_min;
    // если > 0 - игнорирует аналоговые изменения датчика больше этого значения как сбойные (два таких значения подряд будут пропущены)
    uint16_t _analog_filter_max;
    #pragma pack(pop)

    // --- Объявление параметров (автоматическая регистрация) ---

    // последние аналоговые показания
    MWOS_PARAM(9, value, PARAM_INT32, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, sensorsCount);
    // от какого значения - считать value_bin как бинарная единица
    MWOS_PARAM(10, fromValue, PARAM_INT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // до какого значения - считать value_bin как бинарная единица
    MWOS_PARAM(11, toValue, PARAM_INT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // отправлять аналоговые значения: 0 = только при изменении бинарного значения, 1 = при изменении аналогового значения (с учетом фильтров)
    MWOS_PARAM(12, analogSend, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // аналоговые показания, только если 0. Если 1 - бинарные (при =1 работает полностью аналогично датчику MWOSSensorBin)
    MWOS_PARAM(14, sensorDigital, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // игнорирует аналоговые изменения датчика меньше этого значения
    MWOS_PARAM(15, analogFilterMin, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // если > 0 - игнорирует аналоговые изменения датчика больше этого значения как сбойные (два таких значения подряд будут пропущены)
    MWOS_PARAM(16, analogFilterMax, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // код ошибки для последней операции чтения показаний датчика
    MWOS_PARAM(17, error, PARAM_INT8, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // вычислять для датчика среднее арифметическое с предыдущим значением
    MWOS_PARAM(18, average, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    MWOSSensorAnalog() : MWOSSensorBase<sensorsCount>() {
        _analog_send = 0;
        _sensor_digital = 0;
        _average = 0;
        _errorCode = 0;
        _analog_filter_min = 0;
        _analog_filter_max = 0;
        _fromValue = 0;
        _toValue = 0;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                _fromValue = MWOSModule::loadValueInt(_fromValue, p_fromValue, 0);
                _toValue = MWOSModule::loadValueInt(_toValue, p_toValue, 0);
                _analog_send = MWOSModule::loadValueInt(_analog_send, p_analogSend, 0);
                _sensor_digital = MWOSModule::loadValueInt(_sensor_digital, p_sensorDigital, 0);
                _analog_filter_min = MWOSModule::loadValueInt(_analog_filter_min, p_analogFilterMin, 0);
                _analog_filter_max = MWOSModule::loadValueInt(_analog_filter_max, p_analogFilterMax, 0);
                _average = MWOSModule::loadValueInt(_average, p_average, 0);
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_fromValue:
                        _fromValue = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_fromValue);
                        break;
                    case id_toValue:
                        _toValue = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_toValue);
                        break;
                    case id_analogSend:
                        _analog_send = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_analogSend);
                        break;
                    case id_sensorDigital:
                        _sensor_digital = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_sensorDigital);
                        break;
                    case id_analogFilterMin:
                        _analog_filter_min = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_analogFilterMin);
                        break;
                    case id_analogFilterMax:
                        _analog_filter_max = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_analogFilterMax);
                        break;
                    case id_average:
                        _average = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_average);
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
                    case id_value: {
                        if (data.param_index < 0 || data.param_index >= sensorsCount) {
                            setError(1);
                            data.setValueInt(0);
                        } else {
                            data.setValueInt(_value[data.param_index]);
                        }
                    } break;
                    case id_fromValue:
                        data.setValueInt(_fromValue);
                        break;
                    case id_toValue:
                        data.setValueInt(_toValue);
                        break;
                    case id_analogSend:
                        data.setValueInt(_analog_send);
                        break;
                    case id_sensorDigital:
                        data.setValueInt(_sensor_digital);
                        break;
                    case id_analogFilterMin:
                        data.setValueInt(_analog_filter_min);
                        break;
                    case id_analogFilterMax:
                        data.setValueInt(_analog_filter_max);
                        break;
                    case id_error:
                        data.setValueInt(_errorCode);
                        break;
                    case id_average:
                        data.setValueInt(_average);
                        break;
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

    /**
    * Установить код ошибки
    */
    void setError(uint8_t errorCode) {
        if (_errorCode != errorCode) {
            MWOSModuleBase::SetParamChanged(&p_error, 0, true);
            if (errorCode != 0) {
                MW_LOG_MODULE(this); MW_LOG(F("ERROR CODE: ")); MW_LOG_LN(errorCode);
            }
        }
        _errorCode = errorCode;
    }

    /**
    * Получить аналоговое значение датчика
    */
    int64_t getValueAnalog(int16_t arrayIndex = 0) {
        MWValue data;
        data.param_id = id_value;
        data.param_index = arrayIndex;
        onEvent(EVENT_GET_VALUE, data);
        return data.toInt();
    }

    /**
    * @return Процент текущих показаний, между fromValue и toValue
    */
    float getValuePercent(uint16_t arrayIndex) {
        if (arrayIndex < 0 || arrayIndex >= sensorsCount || _fromValue >= _toValue) return 0;
        float percent = (_value[arrayIndex] - _fromValue) * 100;
        return percent / (float)(_toValue - _fromValue);
    }

    virtual bool readBoolValue(uint16_t arrayIndex) override {
        _errorCode = 0;
        if (_sensor_digital) return readDigitalValue(arrayIndex); // это бинарный датчик - просто считаем бинарное значение

        int32_t newValue = readAnalogValue(arrayIndex);
        if (_errorCode != 0) { // была ошибка чтения
            newValue = _value[arrayIndex];
        }

        bool analogChanged = MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed;

        if (MWOSSensorBase<sensorsCount>::_valueCount > 3) {
            int32_t changedValue = newValue - _value[arrayIndex];
            changedValue = abs(changedValue);

            if (_analog_filter_min > 0 && _analog_filter_min > changedValue) {
                // слишком слабо изменилось значение датчика - игнорируем
                MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed = 0;
                #if (defined(MWOS_LOG_SENSOR) && MWOS_LOG_SENSOR > 0)
                MW_LOG_MODULE(this, &p_value, arrayIndex); MW_LOG(F("Skip min>change=value: ")); MW_LOG(_analog_filter_min); MW_LOG('>'); MW_LOG(changedValue); MW_LOG('='); MW_LOG_LN(newValue);
                #endif
                newValue = _value[arrayIndex];
            }

            if (newValue != _value[arrayIndex]) {
                MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed = 1;
                analogChanged = true;
            }

            if (analogChanged) {
                if (_analog_filter_max > 0 && _analog_filter_max < changedValue) {
                    // слишком сильно изменилось значение датчика - дождемся повтора измерения
                    MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed = 0;
                    #if (defined(MWOS_LOG_SENSOR) && MWOS_LOG_SENSOR > 0)
                    MW_LOG_MODULE(this, &p_value, arrayIndex); MW_LOG(F("Skip max<change=value: ")); MW_LOG(_analog_filter_max); MW_LOG('<'); MW_LOG(changedValue); MW_LOG('='); MW_LOG_LN(newValue);
                    #endif
                    newValue = _value[arrayIndex];
                }

                if (_average) _value[arrayIndex] = (_value[arrayIndex] + newValue) / 2;
                else _value[arrayIndex] = newValue;
            }
        } else {
            _value[arrayIndex] = newValue;
            MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed = 1;
        }

        bool newValueBin = (newValue >= _fromValue) && (newValue <= _toValue);
        bool binChanged = (newValueBin != MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].bin_value);

        if ((_analog_send == 0 && binChanged) || (_analog_send == 1 && analogChanged)) {
            MWOSModuleBase::SetParamChanged(&p_value, arrayIndex, true);
        }

        #if (defined(MWOS_LOG_SENSOR) && MWOS_LOG_SENSOR > 3)
        MW_LOG_MODULE(this, &p_value, arrayIndex); MW_LOG('='); MW_LOG(newValue); MW_LOG(':'); MW_LOG_LN(newValueBin);
        #endif

        return newValueBin;
    }

    /**
    * Опросить аналоговый датчик для получения новых показаний
    */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        #if (defined(MWOS_LOG_SENSOR))
        MW_LOG_MODULE(this, &p_value, arrayIndex); MW_LOG_LN(F("readAnalogValue error!"));
        #endif
        return 0;
    }

    /**
    * Опросить цифровой датчик для получения новых показаний
    */
    virtual int32_t readDigitalValue(int16_t arrayIndex) {
        #if (defined(MWOS_LOG_SENSOR))
        MW_LOG_MODULE(this, &p_value, arrayIndex); MW_LOG_LN(F("readDigitalValue error!"));
        #endif
        return 0;
    }
};

#endif