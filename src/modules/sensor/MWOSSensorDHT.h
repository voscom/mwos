#ifndef MWOS35_MWOSSENSORDHT_H
#define MWOS35_MWOSSENSORDHT_H

#include "core/MWOSModule.h"
#include "core/MWOSSensorAnalog.h"
#include <dhtnew.h>

/**
* Датчик температуры и влажности DHT (Версия MWOS3.5)
*
* Поддерживаемые модели: DHT11, DHT22, DHT33, DHT44, AM2301, AM2302, AM2303
* Два параметра value:
*   0 - температура (C * 10)
*   1 - влажность (% * 10)
* Использует библиотеку: https://github.com/RobTillaart/DHTNew.git
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение датчиков).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<class MWOSSensorAnalogType = MWOSSensorAnalog<2>>
class MWOSSensorDHT : public MWOSSensorAnalogType {
public:
    // номер порта для датчика DHT
    MWOS_PIN_INT _pin = -1;
    // указатель на объект библиотеки DHT
    DHTNEW *_dht = nullptr;

    // --- Объявление параметров (автоматическая регистрация) ---

    // порт для датчика DHT
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);

    MWOSSensorDHT() : MWOSSensorAnalogType() {
        MWOSModuleBase::setName((char *) F("dht"));
    }

    MWOSSensorDHT(MWOS_PIN_INT pin) : MWOSSensorDHT() {
        _pin = pin;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                MWOS_PIN_INT pin = MWOSModule::loadValueInt(_pin, p_pin, 0);
                if (pin != _pin) {
                    _pin = pin;
                    if (_dht != nullptr) delete _dht;
                    if (_pin > -1) _dht = new DHTNEW(_pin);
                } else {
                    if (_dht == nullptr && _pin > -1) _dht = new DHTNEW(_pin);
                }
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_pin: {
                        MWOS_PIN_INT pin = data.toInt();
                        if (pin != _pin) {
                            _pin = pin;
                            if (_dht != nullptr) delete _dht;
                            if (_pin > -1) _dht = new DHTNEW(_pin);
                        }
                        MWOSModuleBase::SetParamChanged(&p_pin);
                    } break;
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
                        data.setValueInt(_pin);
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

    /**
    * Опросить аналоговый датчик для получения новых показаний
    */
    virtual int32_t readAnalogValue(int16_t arrayIndex) override {
        MWOSSensorAnalogType::_errorCode = 99;
        if (_pin < 0 || _dht == nullptr) return 0;

        float v = DHTLIB_INVALID_VALUE;
        if (arrayIndex == 0) {
            _dht->read();
            v = _dht->getTemperature();
        } else {
            v = _dht->getHumidity();
        }

        if (v == DHTLIB_INVALID_VALUE) {
            MWOSSensorAnalogType::_errorCode = 98;
            return 0;
        }

        return round(v * 10);
    }
};

#endif
