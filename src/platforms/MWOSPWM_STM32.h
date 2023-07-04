#ifndef MWOS3_MWOSHZ_STM32_H
#define MWOS3_MWOSHZ_STM32_H
#include <Arduino.h>
#include "core/MWOSRegulator.h"
#include <HardwareTimer.h>
/**
 * Регулятор частоты и скваженности
 * Поддержка:
 * STM32 - аппаратный таймер
 *
 * Параметры:
 * 0 - 1=включено, 0=выключено
 * 1 - частота hz
 * 2 - скваженность %
 */
class MWOSPWM_STM32: public MWOSRegulator {
public:

    HardwareTimer * tim;
    uint32_t channel;

    MWOSPWM_STM32() : MWOSRegulator() {
        setName((char *) F("hz"));
    }

    MWOSPWM_STM32(uint8_t pin, int64_t fr=0, int64_t pulse=50) : MWOSHz() {
        _pin=pin;
        _value=fr; // частота
        _value2=pulse; // по умолчанию скваженность 50%
    }

    virtual char * getName() {  return (char *) F("hz"); }

    virtual void onInit() {
        MWOSRegulator::onInit();
        if (_pin<255) {
            pinMode(_pin, OUTPUT);

            TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(_pin), PinMap_PWM);
            channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(_pin), PinMap_PWM));
            tim = new HardwareTimer(Instance);
            tim->pause();
        }
    }

    virtual void stop() {
        MWOSRegulator::stop();
        tim->pause();
    }

    virtual void start() {
        MWOSRegulator::start();
        tim->setPWM(channel, _pin, _value, _value2, NULL, NULL);
        tim->getCaptureCompare(channel);
    }

    virtual void restart() {
        //MWOSRegulator::restart();
        MW_LOG_MODULE(this); MW_LOG(F("STM32 quick restart")); MW_LOG(_value); MW_LOG('/'); MW_LOG_LN(_value2);
        tim->setOverflow(_value, HERTZ_FORMAT);
        tim->refresh();
    }


};
#endif //MWOS3_MWOSHZ_STM32_H
