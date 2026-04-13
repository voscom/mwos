#ifndef MWOS35_MWOSSENSORADS1115_H
#define MWOS35_MWOSSENSORADS1115_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include "core/MWOSSensorAnalog.h"
#include <Wire.h>

#ifndef MWOS_SENSOR_PULL_TIME_MS
// минимальное время включения подтяжки на портах перед замером показаний
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

// адрес по умолчанию для ADS1115
#define ADS1115_DEFAULT_ADDRESS (0x48)

// регистр конфигурации
#define ADS1X15_REG_POINTER_CONFIG (0x01)

// задержка конвертации (в мС)
#define ADS1115_CONVERSIONDELAY (100)

// маска регистра указателя
#define ADS1115_REG_POINTER_MASK (0x03)
#define ADS1X15_REG_POINTER_CONVERT (0x00)
#define ADS1115_REG_POINTER_CONFIG (0x01)
#define ADS1X15_REG_POINTER_LOWTHRESH (0x02)
#define ADS1X15_REG_POINTER_HITHRESH (0x03)

// биты конфигурации
#define ADS1115_REG_CONFIG_OS_MASK (0x8000)
#define ADS1115_REG_CONFIG_OS_SINGLE (0x8000)
#define ADS1115_REG_CONFIG_OS_BUSY (0x0000)
#define ADS1115_REG_CONFIG_OS_NOTBUSY (0x8000)
#define ADS1115_REG_CONFIG_MUX_MASK (0x7000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_1 (0x5000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_2 (0x6000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_3 (0x7000)
#define ADS1115_REG_CONFIG_PGA_MASK (0x0E00)
#define ADS1115_REG_CONFIG_PGA_6_144V (0x0000)
#define ADS1115_REG_CONFIG_PGA_4_096V (0x0200)
#define ADS1115_REG_CONFIG_PGA_2_048V (0x0400)
#define ADS1115_REG_CONFIG_PGA_1_024V (0x0600)
#define ADS1115_REG_CONFIG_PGA_0_512V (0x0800)
#define ADS1115_REG_CONFIG_PGA_0_256V (0x0A00)
#define ADS1115_REG_CONFIG_EVENT_MASK (0x0100)
#define ADS1115_REG_CONFIG_EVENT_CONTIN (0x0000)
#define ADS1115_REG_CONFIG_EVENT_SINGLE (0x0100)
#define ADS1115_REG_CONFIG_DR_MASK (0x00E0)
#define ADS1115_REG_CONFIG_DR_8SPS (0x0000)
#define ADS1115_REG_CONFIG_DR_16SPS (0x0020)
#define ADS1115_REG_CONFIG_DR_32SPS (0x0040)
#define ADS1115_REG_CONFIG_DR_64SPS (0x0060)
#define ADS1115_REG_CONFIG_DR_128SPS (0x0080)
#define ADS1115_REG_CONFIG_DR_250SPS (0x00A0)
#define ADS1115_REG_CONFIG_DR_475SPS (0x00C0)
#define ADS1115_REG_CONFIG_DR_860SPS (0x00E0)
#define ADS1X15_REG_CONFIG_CQUE_NONE (0x0003)
#define ADS1115_REG_CONFIG_CLAT_NONLAT (0x0000)
#define ADS1115_REG_CONFIG_CPOL_ACTVLOW (0x0000)
#define ADS1115_REG_CONFIG_CEVENT_TRAD (0x0000)

// таблица усилений
const uint16_t adsGainList[] PROGMEM = {
    ADS1115_REG_CONFIG_PGA_6_144V,
    ADS1115_REG_CONFIG_PGA_4_096V,
    ADS1115_REG_CONFIG_PGA_2_048V,
    ADS1115_REG_CONFIG_PGA_1_024V,
    ADS1115_REG_CONFIG_PGA_0_512V,
    ADS1115_REG_CONFIG_PGA_0_256V
};

// таблица режимов
const uint16_t adsOsModeList[] PROGMEM = {
    ADS1115_REG_CONFIG_OS_SINGLE,
    ADS1115_REG_CONFIG_OS_BUSY,
    ADS1115_REG_CONFIG_OS_NOTBUSY
};

// таблица частот дискретизации
const uint16_t adsRateList[] PROGMEM = {
    ADS1115_REG_CONFIG_DR_8SPS,
    ADS1115_REG_CONFIG_DR_16SPS,
    ADS1115_REG_CONFIG_DR_32SPS,
    ADS1115_REG_CONFIG_DR_64SPS,
    ADS1115_REG_CONFIG_DR_128SPS,
    ADS1115_REG_CONFIG_DR_250SPS,
    ADS1115_REG_CONFIG_DR_475SPS,
    ADS1115_REG_CONFIG_DR_860SPS
};

// таблица режимов работы
const uint16_t modeList[] PROGMEM = {
    ADS1115_REG_CONFIG_EVENT_CONTIN,
    ADS1115_REG_CONFIG_EVENT_SINGLE
};

/**
* Шаблонный класс для аналоговых датчиков на модуле ADS1115 (Версия MWOS3.5)
* 
* Наследуется от MWOSSensorAnalog, добавляет настройки I2C и параметры АЦП.
* Поддерживает настройку усиления, частоты дискретизации и режима работы.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение АЦП).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT ADS1115sensorsCount = 4, class MWOSSensorAnalogType = MWOSSensorAnalog<ADS1115sensorsCount>>
class MWOSSensorADS1115 : public MWOSSensorAnalogType {
public:
    // указатель на объект I2C
    TwoWire *_wire;

    #pragma pack(push, 1)
    // адрес устройства I2C
    uint8_t _addr;
    // текущий индекс канала
    uint8_t _index;
    // конфигурация ADS1115
    uint16_t _configADS;
    // последние значения замеров
    uint16_t lastV[ADS1115sensorsCount];
    #pragma pack(pop)

    // --- Объявление параметров (автоматическая регистрация) ---
    
    // усиление (TWOTHIRDS;ONE;TWO;FOUR;EIGHT;SIXTEEN)
    MWOS_PARAM_FF(22, gain, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "TWOTHIRDS;ONE;TWO;FOUR;EIGHT;SIXTEEN");
    // режим работы (CONTIN;SINGLE)
    MWOS_PARAM_FF(23, mode, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "CONTIN;SINGLE");
    // частота дискретизации (8SPS;16SPS;32SPS;64SPS;128SPS;250SPS;475SPS;860SPS)
    MWOS_PARAM_FF(24, rate, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "8SPS;16SPS;32SPS;64SPS;128SPS;250SPS;475SPS;860SPS");
    // режим одиночного преобразования (SINGLE;BUSY;NOTBUSY)
    MWOS_PARAM_FF(25, osMode, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "SINGLE;BUSY;NOTBUSY");

    MWOSSensorADS1115(TwoWire *wireInstance = &Wire, uint8_t addr = 0) : MWOSSensorAnalogType() {
        _wire = wireInstance;
        _index = 0;
        _addr = ADS1115_DEFAULT_ADDRESS + addr;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            case EVENT_INIT: {
                uint8_t gainNum = MWOSModule::loadValueInt(2, p_gain, 0);
                uint16_t gain = pgm_read_word_near(&adsGainList[gainNum]);
                uint8_t osModeNum = MWOSModule::loadValueInt(0, p_osMode, 0);
                uint16_t osMode = pgm_read_word_near(&adsOsModeList[osModeNum]);
                uint8_t rateNum = MWOSModule::loadValueInt(4, p_rate, 0);
                uint16_t rate = pgm_read_word_near(&adsRateList[rateNum]);
                uint8_t modeNum = MWOSModule::loadValueInt(1, p_mode, 0);
                uint16_t mode = pgm_read_word_near(&modeList[modeNum]);

                _configADS = ADS1X15_REG_CONFIG_CQUE_NONE |
                             ADS1115_REG_CONFIG_CLAT_NONLAT |
                             ADS1115_REG_CONFIG_CPOL_ACTVLOW |
                             ADS1115_REG_CONFIG_EVENT_SINGLE |
                             ADS1115_REG_CONFIG_CEVENT_TRAD;
                _configADS |= gain;
                _configADS |= rate;
                _configADS |= ADS1115_REG_CONFIG_OS_SINGLE;
                _index = 0;
                startADCReading(_index);
            } break;

            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_gain:
                    case id_mode:
                    case id_rate:
                    case id_osMode:
                        // Перенастройка при изменении параметров
                        MWOSModuleBase::SetParamChanged(&p_gain);
                        break;
                    default:
                        break;
                }
            } break;

            case EVENT_UPDATE: {
                if (MWOSSensorAnalogType::_always_off == 0 && timeout.isTimeout() && conversionComplete()) {
                    lastV[_index] = readRegister(ADS1X15_REG_POINTER_CONVERT);
                    _index++;
                    if (_index >= ADS1115sensorsCount) _index = 0;
                    startADCReading(_index);
                }
            } break;

            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_gain:
                    case id_mode:
                    case id_rate:
                    case id_osMode:
                        // Возврат настроек
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
        return lastV[arrayIndex];
    }

    /**
    * Проверка завершения конвертации
    * @return true - конвертация завершена
    */
    bool conversionComplete() {
        return (readRegister(ADS1X15_REG_POINTER_CONFIG) & 0x8000) != 0;
    }

private:
    // таймаут ожидания
    MWTimeout timeout;

    /**
    * Запустить чтение АЦП для указанного канала
    */
    void startADCReading(uint8_t channel) {
        uint16_t mux = 0;
        switch (channel) {
            case 0: mux = ADS1115_REG_CONFIG_MUX_SINGLE_0; break;
            case 1: mux = ADS1115_REG_CONFIG_MUX_SINGLE_1; break;
            case 2: mux = ADS1115_REG_CONFIG_MUX_SINGLE_2; break;
            case 3: mux = ADS1115_REG_CONFIG_MUX_SINGLE_3; break;
        }
        if (mux == 0) return;

        uint16_t config = _configADS;
        config |= mux;

        MWOSSensorAnalogType::setError(writeRegister(ADS1X15_REG_POINTER_CONFIG, config));
        writeRegister(ADS1X15_REG_POINTER_HITHRESH, 0x8000);
        writeRegister(ADS1X15_REG_POINTER_LOWTHRESH, 0x0000);
        timeout.startMS(25);
    }

    /**
    * Записать 16 бит в регистр
    */
    uint8_t writeRegister(uint8_t reg, uint16_t value) {
        _wire->beginTransmission(_addr);
        _wire->write((uint8_t)reg);
        _wire->write((uint8_t)(value >> 8));
        _wire->write((uint8_t)(value & 0xFF));
        return _wire->endTransmission();
    }

    /**
    * Прочитать 16 бит из регистра
    */
    uint16_t readRegister(uint8_t reg) {
        _wire->beginTransmission(_addr);
        _wire->write((uint8_t)reg);
        _wire->endTransmission();
        _wire->requestFrom(_addr, (uint8_t)2);
        return (int16_t)((_wire->read() << 8) | _wire->read());
    }
};

#endif