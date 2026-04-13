#ifndef MWOS35_MWOSKEYFEEDBACK_H
#define MWOS35_MWOSKEYFEEDBACK_H

#include "MWOSKeyAnalog.h"
#include "core/adlib/MWTimeout.h"

/**
* Ключ управления мощностью (Версия MWOS3.5)
*
* Управляется заданным датчиком. При изменении аналогового значения датчика изменяет мощность ключа.
* Датчик и индекс значения задаются в параметрах модуля. Номер параметра датчика — всегда 9 (аналоговое значение).
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (отслеживание датчика, регулировка мощности).
*   EVENT_GET_VALUE - возврат значений через switch.
* @tparam keysCount    Количество однотипных ключей
* @tparam MWOSKeyForFeedback   Тип ключа - любой наследник MWOSKeyBase<keysCount> (по умолчанию - MWOSKey<keysCount>)
*/
template<MWOS_PARAM_INDEX_UINT keysCount, class MWOSKeyForFeedback = MWOSKeyAnalog<keysCount>>
class MWOSKeyFeedback : public MWOSKeyForFeedback {
public:
    // --- Локальные переменные ---

    // указатель на модуль датчика
    MWOSModuleBase *_sensor = nullptr;
    // ID модуля датчика
    MWOS_PARAM_INDEX_UINT _sensor_id = -1;
    // индексы параметра 9 датчика для каждого ключа
    MWOS_PARAM_INDEX_UINT _pIndex[keysCount];
    // таймеры для каждого ключа
    MWTimeout<uint16_t> _timeoutKey[keysCount];
    // минимальное значение мощности
    int16_t _powerMin = 1;
    // максимальное значение мощности
    int16_t _powerMax = 255;
    // время между изменениями мощности (плавность регулировки) [мс]
    uint16_t _changeTimeMS = 20;
    // изменение мощности за один шаг
    int16_t _stepPower = 1;

    // --- Объявление параметров (автоматическая регистрация) ---

    // датчик для управления этим переключателем
    MWOS_PARAM(21, sensorModuleId, MWOS_PARAM_INT_PTYPE, PARAM_TYPE_MODULE_ID + PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // индексы параметра 9 этого датчика (если там много однотипных датчиков, то каждый из этих датчиков может управлять своим ключом в этом модуле)
    MWOS_PARAM(22, sensorIndex, MWOS_PARAM_INDEX_UINT_PTYPE, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);
    // минимальное и максимальное значение мощности
    MWOS_PARAM(30, power, PARAM_INT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 2);
    // время между изменениями мощности (плавность регулировки) [мс]
    MWOS_PARAM(31, changeTimeMS, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // изменение мощности за один шаг
    MWOS_PARAM(32, stepPower, PARAM_INT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    /**
    * Конструктор по умолчанию
    */
    MWOSKeyFeedback() : MWOSKeyForFeedback() {
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index] = index;
        }
    }

    /**
    * Создать ключи из массива в PROGMEM
    * @param pins ссылка на область PROGMEM с номерами портов по умолчанию
    */
    MWOSKeyFeedback(const uint8_t * pins) : MWOSKeyForFeedback(pins) {
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index] = index;
        }
    }

    /**
    * Создать одиночный ключ
    * @param pin номер порта первого ключа
    * @param sensor_id ID аналогового датчика регулятора
    */
    MWOSKeyFeedback(MWOS_PIN_INT pin, int16_t sensor_id = -1) : MWOSKeyForFeedback(pin) {
        _sensor_id = sensor_id;
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index] = index;
        }
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                _sensor = nullptr;
                _stepPower = MWOSModule::loadValueInt(_stepPower, p_stepPower, 0);
                _sensor_id = MWOSModule::loadValueInt(_sensor_id, p_sensorModuleId, 0);
                _powerMin = MWOSModule::loadValueInt(_powerMin, p_power, 0);
                _powerMax = MWOSModule::loadValueInt(_powerMax, p_power, 1);
                _changeTimeMS = MWOSModule::loadValueInt(_changeTimeMS, p_changeTimeMS, 0);
                for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                    _pIndex[index] = MWOSModule::loadValueInt(_pIndex[index], p_sensorIndex, index);
                }
                _timeoutKey[0].start(500); // перечитаем датчики через 0.5 сек (чтобы они уже имели правильные показания)
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_sensorModuleId:
                        _sensor_id = data.toInt();
                        _sensor = nullptr; // сбросим ссылку на датчик
                        MWOSModuleBase::SetParamChanged(&p_sensorModuleId);
                        break;
                    case id_sensorIndex:
                        _pIndex[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_sensorIndex, data.param_index);
                        break;
                    case id_power:
                        if (data.param_index == 0) {
                            _powerMin = data.toInt();
                        } else {
                            _powerMax = data.toInt();
                        }
                        MWOSModuleBase::SetParamChanged(&p_power, data.param_index);
                        break;
                    case id_changeTimeMS:
                        _changeTimeMS = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_changeTimeMS);
                        break;
                    case id_stepPower:
                        _stepPower = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_stepPower);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                if (_sensor == nullptr) {
                    if (_timeoutKey[0].isStarted() && _timeoutKey[0].isTimeout()) {
                        loadSensors();
                    }
                    return;
                }

                MWOSParam * sensorParam9 = _sensor->getParam(9);
                if (sensorParam9 != nullptr) {
                    for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                        if (_pIndex[index] >= 0 && _pIndex[index] < sensorParam9->arrayCount()) {
                            updateKey(_pIndex[index], index);
                        }
                    }
                }
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_sensorModuleId:
                        data.setValueInt(_sensor_id);
                        return;
                    case id_sensorIndex:
                        data.setValueInt(_pIndex[data.param_index]);
                        return;
                    case id_power:
                        if (data.param_index == 1) {
                            data.setValueInt(_powerMax);
                        } else {
                            data.setValueInt(_powerMin);
                        }
                        return;
                    case id_changeTimeMS:
                        data.setValueInt(_changeTimeMS);
                        return;
                    case id_stepPower:
                        data.setValueInt(_stepPower);
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSKeyForFeedback::onEvent(modeEvent, data);
    }

    /**
    * Обновить мощность в зависимости от показания датчика
    * @param sensorIndex индекс датчика
    * @param keyIndex индекс ключа
    */
    void updateKey(int16_t sensorIndex, int16_t keyIndex) {
        if (!_timeoutKey[keyIndex].isStarted()) {
            if (MWOSKeyForFeedback::_value[keyIndex] < _powerMin) {
                MWOSKeyForFeedback::_value[keyIndex] = _powerMin;
            }
            if (MWOSKeyForFeedback::_value[keyIndex] > _powerMax) {
                MWOSKeyForFeedback::_value[keyIndex] = _powerMax;
            }
            MWOSKeyForFeedback::turn(1, keyIndex, false);
            _timeoutKey[keyIndex].start(_changeTimeMS);
        } else if (!_timeoutKey[keyIndex].isTimeout()) {
            int32_t sensorValue = _sensor->getValueByParamId((int64_t) 0, 9, sensorIndex);
            int32_t sensorValueMin = _sensor->getValueByParamId((int64_t) 0,10, 0);
            int32_t sensorValueMax = _sensor->getValueByParamId((int64_t) 0,11, 0);

            if (sensorValue > sensorValueMax) {
                MWOSKeyForFeedback::_value[keyIndex] -= _stepPower;
                if (MWOSKeyForFeedback::_value[keyIndex] < _powerMin) {
                    MWOSKeyForFeedback::_value[keyIndex] = _powerMin;
                }
                if (MWOSKeyForFeedback::_value[keyIndex] > _powerMax) {
                    MWOSKeyForFeedback::_value[keyIndex] = _powerMax;
                }
                MWOSKeyForFeedback::setKeyValue(0, keyIndex);
            } else if (sensorValue < sensorValueMin) {
                MWOSKeyForFeedback::_value[keyIndex] += _stepPower;
                if (MWOSKeyForFeedback::_value[keyIndex] < _powerMin) {
                    MWOSKeyForFeedback::_value[keyIndex] = _powerMin;
                }
                if (MWOSKeyForFeedback::_value[keyIndex] > _powerMax) {
                    MWOSKeyForFeedback::_value[keyIndex] = _powerMax;
                }
                MWOSKeyForFeedback::setKeyValue(0, keyIndex);
            }
            _timeoutKey[keyIndex].start(_changeTimeMS);
        }
    }

    /**
    * Загрузить ссылку на модуль датчика
    */
    void loadSensors() {
        _timeoutKey[0].stop();

        _sensor_id = MWOSModule::loadValueInt(_sensor_id, p_sensorModuleId, 0);
        if (_sensor_id < 2) {
            return;
        }

        _sensor = mwos.getModule(_sensor_id);
        if (_sensor == nullptr) {
            return;
        }

        MWOSParam * sensorParam0 = _sensor->getParam(0);
        if (sensorParam0 == nullptr ||
            !sensorParam0->IsGroup(PARAM_TYPE_READONLY) ||
            sensorParam0->valueType != PARAM_BITS1) {
            _sensor = nullptr; // удалим не датчики
            return;
        }

        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            if (_pIndex[index] >= 0 && _pIndex[index] < sensorParam0->arrayCount()) {
                int8_t nowSensorValue = _sensor->getValue((int64_t) 0, sensorParam0, _pIndex[index]);
            }
        }
    }
};

#endif