#ifndef MWOS35_MWOSTIMEDS3231_H
#define MWOS35_MWOSTIMEDS3231_H

#include "MWOSTime.h"
#include "core/MWOSDevI2C.h"

#define DS1307_ADDRESS 0x68 ///< I2C address for DS1307
#define DS3231_CONTROLREG 0x0E ///< Status register
#define DS3231_STATUSREG 0x0F ///< Status register
#define DS3231_TIME 0x00      ///< Time register

/**
* Модуль учета времени для работы с RTC DS3231 (Версия MWOS3.5)
*
* Принцип работы как в библиотеке https://github.com/adafruit/RTClib.git
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка времени из RTC при старте.
*   EVENT_CHANGE - реакция на изменение времени.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSTimeDS3231 : public MWOSTime, public MWOSDevI2C {
public:
    // --- Объявление параметров (автоматическая регистрация) ---

    // регистр коррекции времени
    MWOS_PARAM(10, correction, PARAM_INT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);
    // регистр управления (0b00011100 = 0x1C = 28)
    MWOS_PARAM(11, control, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);

    MWOSTimeDS3231(TwoWire *wireInstance = &Wire) : MWOSTime(), MWOSDevI2C(wireInstance) {
        _addr = DS1307_ADDRESS;
        MWOSModuleBase::setName((char *) F("timeDS3231"));
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем время из RTC при старте)
            case EVENT_INIT: {
                loadTimeFromRTC();
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_correction:
                        write_register(0x10, data.toInt());
                        break;
                    case id_control:
                        write_register(0x0E, data.toInt());
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_correction:
                        data.setValueInt(read_register(0x10));
                        return;
                    case id_control:
                        data.setValueInt(read_register(0x0E));
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSTime::onEvent(modeEvent, data);
    }

    /**
    * Загрузить время из RTC DS3231
    */
    bool loadTimeFromRTC() {
        uint8_t buffer[7];
        buffer[0] = 0;
        if (write_then_read(buffer, 1, buffer, 7)) {
            DateTime dt(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5] & 0x7F),
                        bcd2bin(buffer[4]), bcd2bin(buffer[2]), bcd2bin(buffer[1]),
                        bcd2bin(buffer[0] & 0x7F));
            setTime(dt.unixtime());
            return true;
        }
        return false;
    }

    /**
    * Сохранить время в RTC DS3231
    */
    void saveTimeToRTC() {
        DateTime dt(getTime(false));
        uint8_t buffer[8] = {DS3231_TIME,
                             bin2bcd(dt.second()),
                             bin2bcd(dt.minute()),
                             bin2bcd(dt.hour()),
                             bin2bcd(dowToDS3231(dt.dayOfTheWeek())),
                             bin2bcd(dt.day()),
                             bin2bcd(dt.month()),
                             bin2bcd(dt.year() - 2000U)};
        if (writeI2C(buffer, 8)) {
            uint8_t statreg = read_register(DS3231_STATUSREG);
            statreg &= ~0x80; // flip OSF bit
            write_register(DS3231_STATUSREG, statreg);
        }
    }

    /**
    * Конвертировать день недели для DS3231
    * @param d День недели (0=Воскресенье, 6=Суббота)
    * @return Конвертированное значение (1=Понедельник, 7=Воскресенье)
    */
    static uint8_t dowToDS3231(uint8_t d) {
        return d == 0 ? 7 : d;
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif
