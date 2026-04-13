#ifndef MWOS35_MWOSKEYSCHEDULE_H
#define MWOS35_MWOSKEYSCHEDULE_H

#include "MWOSKey.h"
#include "core/adlib/MWTimeout.h"
#include "core/adlib/MWBitsMaskStat.h"

/**
* Ключи с расширенной функциональностью (Версия MWOS3.5)
*
* Может сам выключаться и/или включаться через таймаут.
* Может включаться ежедневно в заданное время (минут с полуночи).
* В эту минуту суток принудительно включает ключ, если он выключен.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (таймеры, расписание).
*   EVENT_GET_VALUE - возврат значений через switch.
* @tparam keysCount    Количество однотипных ключей
* @tparam MWOSKeyForSchedule   Тип ключа - любой наследник MWOSKeyBase<keysCount> (по умолчанию - MWOSKey<keysCount>)
*/
template<MWOS_PARAM_INDEX_UINT keysCount, class MWOSKeyForSchedule = MWOSKey<keysCount>>
class MWOSKeySchedule : public MWOSKeyForSchedule {
public:
    // --- Локальные переменные ---

    // максимальное время включения ключа [сек/1000]
    uint32_t _timeon[keysCount];
    // максимальное время выключения ключа [сек/1000]
    uint32_t _timeoff[keysCount];
    // время для расписания (минут с полуночи)
    uint16_t _schedule[keysCount];
    // как реагировать на расписание (0=Без расписания;1=Выключать;2=Включать;3=Переключать)
    uint8_t _scheduleTurn[keysCount];
    // таймеры для каждого ключа
    MWTimeout<uint32_t> lastTimeOut[keysCount];
    // индекс следующего ключа для опроса
    uint16_t nextKeyIndex = 0;
    // флаг выполнения расписания для каждого ключа
    MWBitsMaskStat<keysCount> sheduleFlag;

    // --- Объявление параметров (автоматическая регистрация) ---

    // максимальное время включения ключа [сек/1000]
    MWOS_PARAM(11, timeon, PARAM_UINT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);
    // максимальное время выключения ключа [сек/1000]
    MWOS_PARAM(12, timeoff, PARAM_UINT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);
    // время для расписания (минут с полуночи)
    MWOS_PARAM(13, schedule, PARAM_INT16, PARAM_TYPE_TIME + PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);
    // как реагировать на расписание (0=Без расписания;1=Выключать;2=Включать;3=Переключать)
    MWOS_PARAM_FF(14, scheduleTurn, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount,
                  "Без расписания;Выключать;Включать;Переключать");

    /**
    * Конструктор по умолчанию
    */
    MWOSKeySchedule() : MWOSKeyForSchedule() {
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _timeon[index] = 0;
            _timeoff[index] = 0;
            _schedule[index] = 0;
            _scheduleTurn[index] = 0;
        }
    }

    /**
    * Создать ключи из массива в PROGMEM
    * @param pins ссылка на область PROGMEM с номерами портов по умолчанию
    */
    MWOSKeySchedule(const uint8_t * pins, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT) : MWOSKeySchedule() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            MWOSKeyForSchedule::_pin[i] = pgm_read_byte_near(pins + i);
            if (MWOSKeyForSchedule::_pin[i] == 255) MWOSKeyForSchedule::_pin[i] = -1;
        }
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Создать одиночный ключ
    * @param pin номер порта первого ключа
    */
    MWOSKeySchedule(MWOS_PIN_INT pin, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT) : MWOSKeySchedule() {
        MWOSKeyForSchedule::_pin[0] = pin;
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                    _timeon[index] = MWOSModule::loadValueInt(_timeon[index], p_timeon, index);
                    _timeoff[index] = MWOSModule::loadValueInt(_timeoff[index], p_timeoff, index);
                    _schedule[index] = MWOSModule::loadValueInt(_schedule[index], p_schedule, index);
                    _scheduleTurn[index] = MWOSModule::loadValueInt(_scheduleTurn[index], p_scheduleTurn, index);
                }
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_timeon:
                        _timeon[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_timeon, data.param_index);
                        break;
                    case id_timeoff:
                        _timeoff[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_timeoff, data.param_index);
                        break;
                    case id_schedule:
                        _schedule[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_schedule, data.param_index);
                        break;
                    case id_scheduleTurn:
                        _scheduleTurn[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_scheduleTurn, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                if (MWOSKeyForSchedule::getValueBool(nextKeyIndex) > 0) {
                    if ((_timeon[nextKeyIndex] > 0) && (lastTimeOut[nextKeyIndex].isTimeout())) {
                        turn(0, nextKeyIndex, true);
                    }
                } else {
                    if ((_timeoff[nextKeyIndex] > 0) && (lastTimeOut[nextKeyIndex].isTimeout())) {
                        turn(1, nextKeyIndex, true);
                    }
                }

                // Проверка расписания (только если задано время)
                if (_scheduleTurn[nextKeyIndex] > 0 && MWOSTime::getTimeModule() && MWOSTime::getTimeModule()->IsTimeSetting()) {
                    uint16_t dailyMin = MWOSTime::getTimeModule()->dailyMin();
                    if (dailyMin != _schedule[nextKeyIndex]) {
                        sheduleFlag.setBit(true, nextKeyIndex);
                    } else if (sheduleFlag.getBit(nextKeyIndex)) {
                        turn(_scheduleTurn[nextKeyIndex] - 1, nextKeyIndex, true);
                        sheduleFlag.setBit(false, nextKeyIndex);
                    }
                }

                nextKeyIndex++;
                if (nextKeyIndex >= keysCount) nextKeyIndex = 0;
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_timeon:
                        data.setValueInt(_timeon[data.param_index]);
                        return;
                    case id_timeoff:
                        data.setValueInt(_timeoff[data.param_index]);
                        return;
                    case id_schedule:
                        data.setValueInt(_schedule[data.param_index]);
                        return;
                    case id_scheduleTurn:
                        data.setValueInt(_scheduleTurn[data.param_index]);
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSKeyForSchedule::onEvent(modeEvent, data);
    }

    /**
    * Изменить значение ключа. (В отличие от turnBool не кеширует значение)
    * @param mode 0=выключить, 1=включить, 2=переключить, 3-255=оставить как есть
    * @param index Индекс ключа
    * @param saveToStorage Сохранять в хранилище
    * @return Возвращает состояние порта
    */
    virtual uint8_t turn(uint8_t mode, int16_t index, bool saveToStorage) override {
        uint8_t v = MWOSKeyForSchedule::turn(mode, index, saveToStorage);

        if (MWOSKeyForSchedule::_turn.getBit(index) > 0) {
            if (_timeon[index] > 0) {
                lastTimeOut[index].start(_timeon[index]);
            }
        } else {
            if (_timeoff[index] > 0) {
                lastTimeOut[index].start(_timeoff[index]);
            }
        }

        return v;
    }
};

#endif