#ifndef MWOS3_MWOSPORTSMAIN_H
#define MWOS3_MWOSPORTSMAIN_H
#include "Arduino.h"
#include "core/MWOSPins.h"
#ifndef AVR
#include <functional>
#endif

#if defined(ESP32) || defined(ESP8266)
#include <FunctionalInterrupt.h>
#endif

// доступные порты, необходимо задавать в platform.ini:
/*
build_flags =
    -D CORE_DEBUG_LEVEL=5
    -D PINS_INPUT=15,2,14,16,17,5,18,19,21,3,1,22,23,13,12,14,27,26,25,33,32,34,35,36,39
    -D PINS_INPUT_PULLUP=15,2,14,16,17,18,19,21,3,1,22,23,13,12,14,27,26,25,33,32
    -D PINS_INPUT_PULLDOWN=15,14,16,17,5,18,19,21,3,1,22,23,13,12,14,27,26,25,33,32
    -D PINS_ADC=25,26,27,32,33,12,13,14,15,2,4,34,35,36,39
    -D PINS_OUTPUT=15,2,14,16,17,5,18,19,21,3,1,22,23,13,12,14,27,26,25,33,32
    -D PINS_OPEN_DRAIN=15,14,16,17,18,19,21,3,1,22,23,13,12,14,27,26,25,33,32
    -D PINS_PWM=15,2,14,16,17,5,18,19,21,3,1,22,23,13,12,14,27,26,25,33,32
    -D PINS_DAC=25,26
    -D PINS_RTC=4,2,15,13,12,14,27,26,25,33,32,35,34,39,36
    -D PINS_TOUCH=4,2,15,13,12,14,27,33,32
    -D PINS_BOOT=1,3,2,4,12,13,0,5,14,15
    -D PINS_LED=2
    -D PINS_USB=1,3
*/

#ifndef PINS_INPUT_PULLUP
#define PINS_INPUT_PULLUP
#endif
#ifndef PINS_INPUT_PULLDOWN
#define PINS_INPUT_PULLDOWN
#endif
#ifndef PINS_ADC
#define PINS_ADC
#endif
#ifndef PINS_OPEN_DRAIN
#define PINS_OPEN_DRAIN
#endif
#ifndef PINS_PWM
#define PINS_PWM
#endif
#ifndef PINS_DAC
#define PINS_DAC
#define PINS_NOT_DAC
#endif
#ifndef PINS_RTC
#define PINS_RTC
#endif
#ifndef PINS_TOUCH
#define PINS_TOUCH
#endif
#ifndef PINS_LED
#define PINS_LED    LED_BUILTIN
#endif
#ifndef PINS_USB
#define PINS_USB    SOC_RX0,SOC_TX0
#endif
#ifndef PINS_BOOT
#define PINS_BOOT   PINS_USB
#endif

const char PinsINPUT[] PROGMEM = { PINS_INPUT }; // ОБЯЗАТЕЛЬНО!!! задавать в platform.ini
const char PinsOUTPUT[] PROGMEM = { PINS_OUTPUT }; // ОБЯЗАТЕЛЬНО!!! задавать в platform.ini
const char PinsINPUT_PULLUP[] PROGMEM = { PINS_INPUT_PULLUP };
const char PinsINPUT_PULLDOWN[] PROGMEM = { PINS_INPUT_PULLDOWN };
const char PinsADC[] PROGMEM = { PINS_ADC };
const char PinsOPEN_DRAIN[] PROGMEM = { PINS_OPEN_DRAIN };
const char PinsPWM[] PROGMEM = { PINS_PWM };
const char PinsDAC[] PROGMEM = { PINS_DAC };

const char PinsRTC[] PROGMEM = { PINS_RTC };
const char PinsTOUCH[] PROGMEM = { PINS_TOUCH };
const char PinsBOOT[] PROGMEM = { PINS_BOOT };
const char PinsLED[] PROGMEM = { PINS_LED };
const char PinsUSB[] PROGMEM = { PINS_USB };

/***
 * Блок портов микроконтроллера
 * Может находиться на внешней микросхеме
 */
class MWOSPinsMain : public MWOSPins {
public:

    MWOSPinsMain() : MWOSPins() {
        lastPin=127; // последний пин
    }

    /**
     * Задать режим порта (вход, выход, подтяжка...)
     * @param pinMode   Режим порта
     * @return Успешно, или нет
     */
    virtual bool mode(MW_PIN_MODE mode) {
        if (pin<firstPin || pin>lastPin) return false;
        switch (mode) {
            case MW_PIN_INPUT: {
                if (IsPinInArray((uint8_t *) &PinsINPUT, sizeof(PinsINPUT))) {
                    pinMode(pin,INPUT);
                    return true;
                }
            } break;
            case MW_PIN_INPUT_PULLDOWN: {
                if (IsPinInArray((uint8_t *) &PinsINPUT_PULLDOWN, sizeof(PinsINPUT_PULLDOWN))) {
#if defined(ESP32)
                    pinMode(pin,INPUT_PULLDOWN);
#elif defined(ESP8266)
                    pinMode(pin,INPUT_PULLDOWN_16);
#else
                    pinMode(pin,INPUT);
#endif
                    return true;
                }
            } break;
            case MW_PIN_INPUT_PULLUP: {
                if (IsPinInArray((uint8_t *) &PinsINPUT_PULLUP, sizeof(PinsINPUT_PULLUP))) {
                    pinMode(pin,INPUT_PULLUP);
                    return true;
                }
            } break;
            case MW_PINT_INPUT_ADC: {
                if (IsPinInArray((uint8_t *) &PinsADC, sizeof(PinsADC))) {
                    pinMode(pin,INPUT);
                    return true;
                }
            } break;
            case MW_PIN_OUTPUT: {
                if (IsPinInArray((uint8_t *) &PinsOUTPUT, sizeof(PinsOUTPUT))) {
                    pinMode(pin,OUTPUT);
                    return true;
                }
            } break;
            case MW_PIN_OUTPUT_OPEN_DRAIN: {
                if (IsPinInArray((uint8_t *) &PinsOPEN_DRAIN, sizeof(PinsOPEN_DRAIN))) {
#if defined(ESP32)
                    pinMode(pin,OPEN_DRAIN);
#else
                    pinMode(pin,OUTPUT);
#endif
                    return true;
                }
            } break;
            case MW_PIN_OUTPUT_PWM: {
                if (IsPinInArray((uint8_t *) &PinsPWM, sizeof(PinsPWM))) {
                    pinMode(pin,OUTPUT);
                    return true;
                }
            } break;
            case MW_PIN_OUTPUT_DAC: {
                if (IsPinInArray((uint8_t *) &PinsDAC, sizeof(PinsDAC))) {
#if defined(ESP32)
                    pinMode(pin,ANALOG);
#else
                    pinMode(pin,OUTPUT);
#endif
                    return true;
                }
            } break;
            default: {

            }
        }
        MW_LOG_TIME();  MW_LOG(pin); MW_LOG(F(" pin error mode ")); MW_LOG_LN((uint8_t) mode);
        return false;
    }

    virtual bool write(bool dat) {
        digitalWrite(pin,dat);
        return true;
    }

    virtual bool writePWM(uint16_t dat) {
        analogWrite(pin,dat);
        return true;
    }

    virtual bool writeDAC(uint16_t dat) {
#ifndef PINS_NOT_DAC
        dacWrite(pin,dat);
#endif
        return true;
    }

    virtual bool read() {
        return digitalRead(pin);
    }

    virtual uint32_t readADC() {
        return analogRead(pin);
    }

    virtual int8_t getInterrupt() {
 #if defined(ESP32) || defined(ESP8266)
        return digitalPinToInterrupt(pin);
#endif
        return -1;
    }

    /**
     * Подключить прерывание для порта
     */
    virtual bool attach(voidFuncPtr funcPtr, int mode) {
        int8_t interrupt=getInterrupt();
        if (interrupt<0) return false;
#if defined(ESP8266)
        //attachInterrupt(interrupt,void (*)(void) funcPtr,mode);
#endif
#if defined(ESP32) || defined(ESP8266)
        attachInterrupt(interrupt,funcPtr,mode);
#endif
#ifdef STM32_MCU_SERIES
        //attachInterrupt(interrupt,(voidFuncPtr) funcPtr,mode);
#endif
        return true;
    }

    /**
     * Отключить прерывание для порта
     */
    virtual bool detach() {
#if defined(ESP32) || defined(ESP8266)
        int8_t interrupt=getInterrupt();
		if (interrupt<0) return false;
		detachInterrupt(interrupt);
#endif
        return true;
    }

};


#endif //MWOS3_MWOSPORTSMAIN_H
