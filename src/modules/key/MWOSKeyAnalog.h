#ifndef MWOS35_MWOSKEYANALOG_H
#define MWOS35_MWOSKEYANALOG_H

#include "MWOSKey.h"

/**
* Несколько однотипных ключей с аналоговыми значениями (ЦАП или ШИМ)
* Например: несколько реле на одном блоке или ШИМ-регуляторы
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKeyAnalog : public MWOSKey<keysCount> {
public:
    // --- Локальные переменные ---
    
    // аналоговое значение для каждого ключа
    int16_t _value[keysCount];

    // --- Объявление параметров (автоматическая регистрация) ---
    
    // аналоговое значение (ШИМ 0-255 или ЦАП 0-4095)
    MWOS_PARAM(8, value, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);

    MWOSKeyAnalog() : MWOSKey<keysCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            _value[i] = 0;
        }
    }

    /**
    * Создать ключи из массива в PROGMEM
    * @param pins ссылка на область PROGMEM с номерами портов по умолчанию
    */
    MWOSKeyAnalog(const uint8_t * pins, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT_PWM) : MWOSKeyAnalog() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            MWOSKey<keysCount>::_pin[i] = pgm_read_byte_near(pins + i);
            if (MWOSKey<keysCount>::_pin[i] == 255) MWOSKey<keysCount>::_pin[i] = -1;
        }
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Создать одиночный ключ
    * @param pin номер порта
    */
    MWOSKeyAnalog(MWOS_PIN_INT pin, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT_PWM) : MWOSKeyAnalog() {
        MWOSKey<keysCount>::_pin[0] = pin;
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
                    _value[index] = MWOSModule::loadValueInt(_value[index], p_value, index);
                }
            } break;
            
            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_value:
                        _value[data.param_index] = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_value, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;
            
            // 3. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_value:
                        data.setValueInt(_value[data.param_index]);
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
    * Установить аналоговое значение ключа
    * @param mode Режим вывода (0=выкл, 1=вкл, >1=аналоговое)
    * @param index Индекс ключа
    */
    void setKeyValue(uint8_t mode, int16_t index) {
        if (MWOSKey<keysCount>::_pin[index] >= 0) {
            if (MWOSKey<keysCount>::_pull | MW_PIN_OUTPUT == MW_PIN_OUTPUT_PWM) {
                if (MWOSKey<keysCount>::getValueBool(index)) {
                    mwos.writeValuePWM(MWOSKey<keysCount>::_pin[index], _value[index]);
                } else {
                    if (mode > 0) mwos.writeValuePWM(MWOSKey<keysCount>::_pin[index], 255);
                    else mwos.writeValuePWM(MWOSKey<keysCount>::_pin[index], 0);
                    mwos.writeValueDigital(MWOSKey<keysCount>::_pin[index], mode);
                }
            } else if (MWOSKey<keysCount>::_pull | MW_PIN_OUTPUT == MW_PIN_OUTPUT_DAC) {
                if (MWOSKey<keysCount>::getValueBool(index)) {
                    mwos.writeValueDAC(MWOSKey<keysCount>::_pin[index], _value[index]);
                } else {
                    mwos.writeValueDAC(MWOSKey<keysCount>::_pin[index], 0);
                    mwos.writeValueDigital(MWOSKey<keysCount>::_pin[index], mode);
                }
            } else {
                mwos.writeValueDigital(MWOSKey<keysCount>::_pin[index], mode);
            }
        }
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
        setKeyValue(v, index);
        return v;
    }
};

#endif
