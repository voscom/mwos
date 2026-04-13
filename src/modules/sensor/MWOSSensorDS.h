#ifndef MWOS35_MWOSSENSORDS_H
#define MWOS35_MWOSSENSORDS_H

#include "core/MWOSModule.h"
#include "core/MWOSSensorAnalog.h"
#include <OneWireNg.h>
#include "OneWireNg_CurrentPlatform.h"

#ifndef MWOS_SENSOR_PULL_TIME_MS
// минимальное время включения подтяжки на портах перед замером показаний
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

/**
* Шаблонный класс для датчиков температуры DS18x20 (Версия MWOS3.5)
*
* Использует библиотеку: https://github.com/pstolarz/OneWireNg.git
* Поддерживает несколько датчиков на одном порту.
* Если список датчиков пуст, производит поиск всех датчиков на линии DS.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (поиск и чтение датчиков).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT sensorsCount, class MWOSSensorAnalogType = MWOSSensorAnalog<sensorsCount>>
class MWOSSensorDS : public MWOSSensorAnalogType {
public:
    // порт для датчиков DS
    MWOS_PIN_INT _pin = -1;
    // порт включения питания для датчиков DS
    MWOS_PIN_INT _pinPower = -1;
    // количество обнаруженных DS-датчиков
    int8_t countDS = 0;
    // шаг поиска датчиков
    int8_t stepDS = 0;
    // указатель на объект OneWire
    OneWireNg *ds = NULL;
    // адреса DS для каждого обнаруженного датчика
    uint64_t _address[sensorsCount];
    // последние значения замеров температуры
    int16_t lastV[sensorsCount];
    // таймер обновления всех датчиков (1/15 сек)
    MWTimeout<uint16_t,15,true> timeoutUpdateAllDS;

    // --- Объявление параметров (автоматическая регистрация) ---

    // порт для датчиков DS
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);
    // порт включения питания для датчиков DS
    MWOS_PARAM(18, power, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);
    // адреса DS для каждого обнаруженного датчика
    MWOS_PARAM(19, address, PARAM_INT64, PARAM_TYPE_READONLY + PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);

    MWOSSensorDS() : MWOSSensorAnalogType() {
        MWOSModuleBase::setName((char *) F("ds"));
    }

    MWOSSensorDS(MWOS_PIN_INT pin, MWOS_PIN_INT pinPower = -1) : MWOSSensorDS() {
        _pin = pin;
        _pinPower = pinPower;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                for (uint16_t i = 0; i < sensorsCount; i++) {
                    _address[i] = MWOSModule::loadValueInt(0, p_address, i);
                }
                _pin = MWOSModule::loadValueInt(_pin, p_pin, 0);
                _pinPower = MWOSModule::loadValueInt(_pinPower, p_power, 0);

                if (_pinPower >= 0) {
                    mwos.mode(_pinPower, MW_PIN_MODE::MW_PIN_OUTPUT);
                    mwos.writeValueDigital(_pinPower, LOW);
                }

                if (_pin < 0) return;

                if (!mwos.mode(_pin, (MW_PIN_MODE)MWOSSensorAnalogType::_sensor_pull)) {
                    _pin = -3;
                    MWOSModule::saveValueInt(-3, p_pin, 0);
                    MWOSModuleBase::SetParamChanged(&p_pin, 0);
                }

                if (ds != NULL) {
                    ds->end();
                    delete ds;
                    ds = nullptr;
                }
                ds = new OneWireNg_CurrentPlatform(_pin, true);
                countDS = 0;
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_pin:
                        _pin = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_pin);
                        break;
                    case id_power:
                        _pinPower = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_power);
                        break;
                    case id_address:
                        // Команда записи адреса - обработка в onReceiveValue
                        break;
                    default:
                        break;
                }
            } break;

            case EVENT_UPDATE: {
                if (countDS == 0 && timeoutUpdateAllDS.isTimeout()) {
                    switch (stepDS) {
                        case 1: {
                            mwos.writeValueDigital(_pinPower, LOW);
                        } break;
                        case 2: {
                            mwos.writeValueDigital(_pinPower, HIGH);
                        } break;
                        case 3: {
                            updateAllDS();
                        } break;
                        default:
                            stepDS = 0;
                    }
                    stepDS++;
                    timeoutUpdateAllDS.start(30);
                }
            } break;

            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_pin:
                        data.setValueInt(_pin);
                        break;
                    case id_power:
                        data.setValueInt(_pinPower);
                        break;
                    case id_address:
                        data.setValueInt(_address[data.param_index]);
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
        if (!isSetAddress(arrayIndex) || _pin < 0) return 0;

        if (measuringIndex == arrayIndex) {
            lastV[arrayIndex] = readDSValue(arrayIndex);
            measuringIndex = -1;
        } else {
            if (measuringIndex == -1 && selectDS(arrayIndex)) {
                measuringIndex = arrayIndex;
            }
        }
        return lastV[arrayIndex];
    }

private:
    // индекс измеряемого датчика
    int16_t measuringIndex = -1;

    /**
    * Ищет все датчики DS на линии и добавляет их в настройки
    */
    void updateAllDS() {
        if (_pin < 0) return;

        union {
            uint64_t addr64 = 0;
            OneWireNg::Id addr;
        };

        int n = 0;
        countDS = 0;

        ds->searchReset();
        while (n < sensorsCount && ds->search(addr) == OneWireNg::EC_MORE) {
            if (((addr[0] == 0x10) || (addr[0] == 0x28) || (addr[0] == 0x22)) &&
                (OneWireNg::crc8(addr, 7) == addr[7])) {
                if (indexOfAddress(addr64) < 0) {
                    int16_t emptyNum = indexOfAddress(0);
                    if (emptyNum < 0) {
                        MWOSSensorAnalogType::setError(3);
                    } else {
                        _address[emptyNum] = addr64;
                        MWOSModule::saveValueInt(addr64, p_address, emptyNum);
                        MWOSModuleBase::SetParamChanged(&p_address, emptyNum);
                    }
                }
                countDS++;
            }
            n++;
        }
    }

    /**
    * Найти порядковый номер адреса в списке адресов
    */
    int16_t indexOfAddress(uint64_t addr64) {
        for (uint16_t i = 0; i < sensorsCount; ++i) {
            if (_address[i] == addr64) return i;
        }
        return -1;
    }

    /**
    * Адрес установлен для этого датчика?
    */
    bool isSetAddress(int16_t numDS) {
        return numDS < sensorsCount && _address[numDS] != 0;
    }

    /**
    * Выбрать текущий DS на линии и включить замер температуры
    */
    bool selectDS(int16_t numDS) {
        if (!isSetAddress(numDS)) return false;

        uint8_t *addDS = (uint8_t *)&_address[numDS];
        if (ds->reset() == 0) {
            return false;
        }
        select(addDS);
        ds->writeByte(0x44, 1);
        return true;
    }

    void select(uint8_t *addDS) {
        ds->writeByte(0x55);
        for (int i = 0; i < 8; i++) {
            ds->writeByte(addDS[i]);
        }
    }

    /**
    * Прочитать результат опроса термодатчика
    */
    int16_t readDSValue(int16_t numDS) {
        uint8_t *addDS = (uint8_t *)&_address[numDS];

        if (ds->reset() == 0) {
            MWOSSensorAnalogType::setError(1);
            return INT16_MAX;
        }

        select(addDS);
        ds->writeByte(0xBE);

        uint8_t data[12];
        for (uint8_t i = 0; i < 9; i++) {
            data[i] = ds->readByte();
        }

        if (OneWireNg::crc8(data, 8) != data[8]) {
            MWOSSensorAnalogType::setError(2);
            return INT16_MAX;
        }

        int16_t raw = (data[1] << 8) | data[0];
        if (addDS[0] == 0x10) {
            raw = raw << 3;
            if (data[7] == 0x10) {
                raw = (raw & 0xFFF0) + 12 - data[6];
            }
        } else {
            uint8_t cfg = (data[4] & 0x60);
            if (cfg == 0x00) raw = raw & ~7;
            else if (cfg == 0x20) raw = raw & ~3;
            else if (cfg == 0x40) raw = raw & ~1;
        }

        float c100 = (((float)raw) / 16.0f) * 100.0f;
        MWOSSensorAnalogType::setError(0);
        return (int16_t)round(c100);
    }
};

#endif