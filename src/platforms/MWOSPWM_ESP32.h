#ifndef MWOS3_MWOSHZ_ESP32_H
#define MWOS3_MWOSHZ_ESP32_H
#include <Arduino.h>
#include "core/MWOSRegulator.h"
/**
 * Регулятор частоты и скваженности
 * Поддержка:
 * ESP32 - аппаратный таймер
 *
 * Параметры:
 * 0 - 1=включено, 0=выключено
 * 1 - частота hz
 * 2 - скваженность %
 */
class MWOSPWM_ESP32: public MWOSRegulator {
public:

    uint8_t _resolution;
    uint8_t _channel;

    // канал ШИМ (аппаратный таймер)
    MWOS_PARAM(4, channel, mwos_param_bits4, mwos_param_option, mwos_param_storage_eeprom, 1);
    // разрешение ШИМ (бит)
    MWOS_PARAM(5, resolution, mwos_param_bits4, mwos_param_option, mwos_param_storage_eeprom, 1);


    MWOSPWM_ESP32() : MWOSRegulator() {
        setName((char *) F("hz"));
    }

    MWOSPWM_ESP32(uint8_t pin, int64_t fr=0, int64_t pulse=50, int8_t channel=-1) : MWOSPWM_ESP32() {
        _pin=pin;
        _value=fr; // частота
        _value2=pulse; // по умолчанию скваженность 50%
        if (channel>=0) _channel=channel;
        else _channel=getNumForName(this);
        _resolution=16;
    }

    virtual void onInit() {
        MWOSRegulator::onInit();
        _channel=loadValue(_channel, &p_channel);
        _resolution=loadValue(_resolution, &p_resolution);
    }

    virtual void stop() {
        MWOSRegulator::stop();
        ledcDetachPin(_pin);
    }

    virtual void start() {
        MWOSRegulator::start();
        ledcSetup(_channel,_value,_resolution); // зададим частоту и битность ШИМ
        ledcAttachPin(_pin, _channel); // ножка процессора
        ledcWrite(_channel, _value2); // зададим значение ШИМ
    }

    virtual void restart() {
        MWOSRegulator::restart();
    }


};
#endif //MWOS3_MWOSHZ_ESP32_H
