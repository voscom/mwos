#ifndef MWOS35_MWOSKEYIR_H
#define MWOS35_MWOSKEYIR_H

#include "core/MWOSKeyBase.h"
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>

/**
* Несколько переключателей, отправляющих команды включения/выключения через один ИК-светодиод
*
* Использует библиотеку:
* https://github.com/crankyoldgit/IRremoteESP8266.git
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKeyIR : public MWOSKeyBase<keysCount> {
public:
    // --- Локальные переменные ---

    // порт ИК-светодиода
    MWOS_PIN_INT _pin = -1;
    // тип IR-пульта для каждого ключа
    decode_type_t _typeIR[keysCount];
    // указатель на объект IRac
    IRac *ac = nullptr;

    // --- Объявление параметров (автоматическая регистрация) ---

    // порт ИК-светодиода
    MWOS_PARAM(6, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);
    // тип IR-пульта для каждого ключа
    MWOS_PARAM(7, typeIR, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, keysCount);

    MWOSKeyIR() : MWOSKeyBase<keysCount>() {
        MWOSModuleBase::setName((char *) F("keyIR"));
    }

    MWOSKeyIR(MWOS_PIN_INT pin) : MWOSKeyIR() {
        _pin = pin;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                MWOS_PIN_INT lastPin = _pin;
                _pin = MWOSModule::loadValueInt(_pin, p_pin, 0);

                if (lastPin != _pin && _pin >= 0) {
                    if (ac != nullptr) delete ac;
                    ac = new IRac(_pin);
                    ac->next.protocol = decode_type_t::DAIKIN;
                    ac->next.model = 1;
                    ac->next.mode = stdAc::opmode_t::kCool;
                    ac->next.celsius = true;
                    ac->next.degrees = 25;
                    ac->next.fanspeed = stdAc::fanspeed_t::kMedium;
                    ac->next.swingv = stdAc::swingv_t::kOff;
                    ac->next.swingh = stdAc::swingh_t::kOff;
                    ac->next.light = false;
                    ac->next.beep = false;
                    ac->next.econo = false;
                    ac->next.filter = false;
                    ac->next.turbo = false;
                    ac->next.quiet = false;
                    ac->next.sleep = -1;
                    ac->next.clean = false;
                    ac->next.clock = -1;
                    ac->next.power = false;
                }

                for (uint16_t i = 0; i < keysCount; ++i) {
                    _typeIR[i] = (decode_type_t) MWOSModule::loadValueInt((uint8_t) _typeIR[i], p_typeIR, i);
                }
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_pin:
                        _pin = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_pin);
                        break;
                    case id_typeIR:
                        _typeIR[data.param_index] = (decode_type_t) data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_typeIR, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_pin:
                        data.setValueInt(_pin);
                        return;
                    case id_typeIR:
                        data.setValueInt((uint8_t) _typeIR[data.param_index]);
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
    * Изменить значение ключа. (В отличие от turnBool не кеширует значение)
    * @param mode 0=выключить, 1=включить, 2=переключить, 3-255=оставить как есть
    * @param index Индекс ключа
    * @param saveToStorage Сохранять в хранилище
    * @return Возвращает состояние порта
    */
    virtual uint8_t turn(uint8_t mode, int16_t index, bool saveToStorage) override {
        uint8_t v = MWOSKeyBase<keysCount>::turn(mode, index, saveToStorage);

        if (ac != nullptr) {
            ac->next.protocol = _typeIR[index];
            ac->next.power = (v > 0);
            ac->sendAc();
        }

        return v;
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif