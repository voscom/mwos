#ifndef MWOS35_MWOSKEY_H
#define MWOS35_MWOSKEY_H

#include "core/MWOSKeyBase.h"

/**
* Несколько однотипных ключей (можно один), переключаемых портами контроллера (пинами)
* Например: Это могут быть несколько реле на одном блоке
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKey : public MWOSKeyBase<keysCount> {
public:
    // --- Локальные переменные ---

    // номера портов для каждого ключа
    MWOS_PIN_INT _pin[keysCount];

    // --- Объявление параметров (автоматическая регистрация) ---

    // подтяжка 'Выход;Открытый коллектор;ШИМ;ЦАП
    MWOS_PARAM_FF(6, pull, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "Выход;Открытый коллектор;ШИМ;ЦАП");
    // порты ключей
    MWOS_PARAM(7, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, keysCount);

    MWOSKey() : MWOSKeyBase<keysCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            _pin[i] = -1;
        }
    }

    /**
    * Создать ключи из массива в PROGMEM
    * @param pins ссылка на область PROGMEM с номерами портов по умолчанию
    */
    MWOSKey(const uint8_t * pins, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT) : MWOSKey() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            _pin[i] = pgm_read_byte_near(pins + i);
            if (_pin[i] == 255) _pin[i] = -1;
        }
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Создать одиночный ключ
    * @param pin номер порта
    */
    MWOSKey(MWOS_PIN_INT pin, MW_PIN_MODE pinOutputMode=MW_PIN_OUTPUT) : MWOSKey() {
        _pin[0] = pin;
        MWOSKeyBase<keysCount>::mode(pinOutputMode);
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                MWOSKeyBase<keysCount>::_pull = MWOSModule::loadValueInt(MWOSKeyBase<keysCount>::_pull, p_pull, 0);
                for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                    _pin[index] = MWOSModule::loadValueInt(_pin[index], p_pin, index);
                    initKey(index);
                }
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_pull:
                        MWOSKeyBase<keysCount>::_pull = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_pull);
                        break;
                    case id_pin:
                        _pin[data.param_index] = data.toInt();
                        initKey(data.param_index);
                        MWOSModuleBase::SetParamChanged(&p_pin, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_pull:
                        data.setValueInt(MWOSKeyBase<keysCount>::_pull);
                        return;
                    case id_pin:
                        data.setValueInt(_pin[data.param_index]);
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSKeyBase<keysCount>::onEvent(modeEvent, data);
    }

    /**
    * Инициализация порта ключа
    */
    void initKey(int16_t index) {
        if (_pin[index] < 0) return;
        if (!mwos.mode(_pin[index], (MW_PIN_MODE) (MWOSKeyBase<keysCount>::_pull | MW_PIN_OUTPUT))) {
            _pin[index] = -3; // этот порт не подходит для заданного режима
            MWOSModule::saveValueInt(-3, p_pin, index);
            MWOSModuleBase::SetParamChanged(&p_pin, index); // сообщим ошибку
        }
    }

    /**
    * Изменить значение ключа. (В отличие от turnBool не кеширует значение)
    * @param mode 0-выключить,1-включить,2-переключить, 3-255-оставить как есть
    * @param index индекс ключа
    * @param saveToStorage Сохранять в хранилище
    * @return Возвращает состояние порта
    */
    virtual uint8_t turn(uint8_t mode, int16_t index, bool saveToStorage) override {
        uint8_t v = MWOSKeyBase<keysCount>::turn(mode, index, saveToStorage);
        if (_pin[index] >= 0) mwos.writeValueDigital(_pin[index], v);
        return v;
    }
};

#endif