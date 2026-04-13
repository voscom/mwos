#ifndef MWOS35_MWOSKEYSWITCH_H
#define MWOS35_MWOSKEYSWITCH_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include "MWOSKey.h"

/**
 * Переключатель (ключ, переключаемый датчиком) (Версия MWOS3.5)
 *
 * Управляется заданным датчиком. При изменении бинарного значения датчика переключает ключ.
 * Датчик и индекс значения задаются в параметрах модуля. Номер параметра датчика — всегда 0 (бинарное значение).
 * Параметры регистрируются автоматически через макросы MWOS_PARAM.
 * Логика обработки данных объединена в onEvent:
 *   EVENT_INIT - загрузка настроек при старте.
 *   EVENT_CHANGE - реакция на изменение конкретного параметра.
 *   EVENT_UPDATE - тиковая обработка (отслеживание датчика, переключение ключа).
 *   EVENT_GET_VALUE - возврат значений через switch.
 *
 * @tparam keysCount    Количество однотипных ключей
 * @tparam MWOSKeyForSwitch   Тип ключа - любой наследник MWOSKeyBase<keysCount> (по умолчанию - MWOSKey<keysCount>)
 */
template<MWOS_PARAM_INDEX_UINT keysCount, class MWOSKeyForSwitch = MWOSKey<keysCount>>
class MWOSKeySwitch : public MWOSKeyForSwitch {
public:
    // --- Локальные переменные ---

    // указатель на модуль датчика
    MWOSModuleBase * _sensor = nullptr;
    // ID модуля датчика
    MWOS_PARAM_INDEX_UINT _sensor_id = -1;
    // индексы параметра 0 датчика для каждого ключа
    MWOS_PARAM_INDEX_UINT _pIndex[keysCount];
    // последние значения датчиков для каждого ключа
    MWBitsMaskStat<keysCount> _lastSensorsValue;
    // таймер инициализации датчиков (1/15 сек)
    MWTimeout<uint16_t,15,true> initTimeout;

    // --- Объявление параметров (автоматическая регистрация) ---

    // датчик для управления этим переключателем
    MWOS_PARAM(21, sensorModuleId, MWOS_PARAM_INT_PTYPE, PARAM_TYPE_MODULE_ID + PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // индексы параметра 0 этого датчика (если там много однотипных датчиков, то каждый из этих датчиков может управлять своим ключом в этом модуле)
    MWOS_PARAM(22, sensorIndex, MWOS_PARAM_INDEX_UINT_PTYPE, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);

    /**
    * Конструктор по умолчанию
    */
    MWOSKeySwitch() : MWOSKeyForSwitch() {
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index] = -1;
        }
    }

    /**
    * Создать ключи из массива в PROGMEM
    * @param pins ссылка на область PROGMEM с номерами портов по умолчанию
    */
    MWOSKeySwitch(const uint8_t * pins, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT) : MWOSKeyForSwitch(pins) {
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index] = -1;
        }
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Создать одиночный ключ
    * @param pin номер порта первого ключа
    */
    MWOSKeySwitch(MWOS_PIN_INT pin, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT) : MWOSKeyForSwitch(pin) {
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index] = -1;
        }
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                _sensor = nullptr;
                _sensor_id = MWOSModule::loadValueInt(_sensor_id, p_sensorModuleId, 0);
                for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                    _pIndex[index] = MWOSModule::loadValueInt(_pIndex[index], p_sensorIndex, index);
                }
                initTimeout.start(4); // перечитаем датчики через 0.3 сек (чтобы они уже имели правильные показания)
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
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                if (_sensor == nullptr) {
                    if (initTimeout.isStarted() && initTimeout.isTimeout()) {
                        loadSensors();
                    }
                    return;
                }

                MWOSParam * sensorParam0 = _sensor->getParam(0);
                if (sensorParam0 != nullptr) {
                    for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                        if (_pIndex[index] >= 0 && _pIndex[index] < sensorParam0->arrayCount()) {
                            int8_t nowSensorValue = _sensor->getValue((int64_t) 0,sensorParam0, _pIndex[index]);
                            if (_lastSensorsValue.getBit(index) != nowSensorValue) {
                                MWOSKeyForSwitch::turn(2, index, true);
                                _lastSensorsValue.setBit(nowSensorValue, index);
                            }
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
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSKeyForSwitch::onEvent(modeEvent, data);
    }

    /**
    * Загрузить ссылку на модуль датчика
    */
    void loadSensors() {
        initTimeout.stop();

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
                _lastSensorsValue.setBit(nowSensorValue, index); // прочитаем текущее значение датчика
            }
        }
    }
};

#endif