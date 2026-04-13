#ifndef MWOS35_MWOSHZSTM32_H
#define MWOS35_MWOSHZSTM32_H

#include "MWOSKey.h"
#include "HardwareTimer.h"

#ifdef STM32_MCU_SERIES

/**
* Регулятор частоты и скважности (Версия MWOS3.5)
*
* Поддержка: STM32 - аппаратный таймер
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSHzSTM32 : public MWOSKey<keysCount> {
public:
    // --- Локальные переменные ---

    // указатель на аппаратный таймер
    HardwareTimer *tim;
    // канал таймера
    uint32_t channel;
    // частота ШИМ
    uint32_t _hz[keysCount];
    // разрешение ШИМ (бит)
    uint8_t _resolution[keysCount];
    // канал ШИМ (аппаратный таймер)
    uint8_t _channel[keysCount];

    // --- Объявление параметров (автоматическая регистрация) ---

    // частота ШИМ
    MWOS_PARAM(30, hz, PARAM_UINT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);
    // канал ШИМ (аппаратный таймер)
    MWOS_PARAM(31, channel, PARAM_BITS4, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);
    // разрешение ШИМ (бит)
    MWOS_PARAM(32, resolution, PARAM_BITS4, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);

    MWOSHzSTM32() : MWOSKey<keysCount>() {
        MWOSModuleBase::setName((char *) F("hz"));
        tim = nullptr;
        channel = 0;
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            _hz[i] = 490;
            _resolution[i] = 8;
            _channel[i] = i;
        }
    }

    /**
    * Создать ключи из массива в PROGMEM
    * @param pins ссылка на область PROGMEM с номерами портов по умолчанию
    */
    MWOSHzSTM32(const uint8_t * pins) : MWOSHzSTM32() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            MWOSKey<keysCount>::_pin[i] = pgm_read_byte_near(pins + i);
            if (MWOSKey<keysCount>::_pin[i] == 255) MWOSKey<keysCount>::_pin[i] = -1;
        }
    }

    /**
    * Создать одиночный ключ
    * @param pin номер порта первого ключа
    */
    MWOSHzSTM32(MWOS_PIN_INT pin) : MWOSHzSTM32() {
        MWOSKey<keysCount>::_pin[0] = pin;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
                    _hz[i] = MWOSModule::loadValueInt(_hz[i], p_hz, i);
                    _channel[i] = MWOSModule::loadValueInt(_channel[i], p_channel, i);
                    _resolution[i] = MWOSModule::loadValueInt(_resolution[i], p_resolution, i);
                }

                if (MWOSKey<keysCount>::_pin[0] >= 0) {
                    TIM_TypeDef *Instance = (TIM_TypeDef *) pinmap_peripheral(digitalPinToPinName(MWOSKey<keysCount>::_pin[0]), PinMap_PWM);
                    channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(MWOSKey<keysCount>::_pin[0]), PinMap_PWM));
                    tim = new HardwareTimer(Instance);
                    if (tim != nullptr) {
                        tim->pause();
                    }
                }
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_hz:
                        _hz[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_hz, data.param_index);
                        break;
                    case id_channel:
                        _channel[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_channel, data.param_index);
                        break;
                    case id_resolution:
                        _resolution[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_resolution, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_hz:
                        data.setValueInt(_hz[data.param_index]);
                        return;
                    case id_channel:
                        data.setValueInt(_channel[data.param_index]);
                        return;
                    case id_resolution:
                        data.setValueInt(_resolution[data.param_index]);
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSKey<keysCount>::onEvent(modeEvent, data);
    }

    /**
    * Изменить значение ключа. (В отличие от turnBool не кеширует значение)
    * @param mode 0=выключить, 1=включить, 2=переключить, 3-255=оставить как есть
    * @param index Индекс ключа
    * @param saveToStorage Сохранять в хранилище
    * @return Возвращает состояние порта
    */
    virtual uint8_t turn(uint8_t mode, int16_t index, bool saveToStorage) override {
        uint8_t v = MWOSKeyBase<keysCount>::turn(mode, index, saveToStorage);

        if (MWOSKey<keysCount>::_pin[index] >= 0 && tim != nullptr) {
            if (MWOSKeyBase<keysCount>::getValueBool(index)) {
                tim->setPWM(channel, MWOSKey<keysCount>::_pin[index], _hz[index], MWOSKey<keysCount>::_periodValue[index], nullptr, nullptr);
                tim->resume();
            } else {
                tim->pause();
            }
        }

        return v;
    }
};

#endif // STM32_MCU_SERIES

#endif