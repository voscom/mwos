#ifndef MWOS3_MWOSPORTSMAIN_H
#define MWOS3_MWOSPORTSMAIN_H

#include "core/MWOSPins.h"
#ifndef AVR
#include <functional>
#endif
#if defined(ESP32) || defined(ESP8266)
#include <FunctionalInterrupt.h>
#endif

#define mwos_pins_pull_down 1
#define mwos_pins_pull_up 2
#define mwos_pins_open_drain 3
#define mwos_pins_analog_input 3

/***
 * Блок портов микроконтроллера
 * Может находиться на внешней микросхемме
 */
class MWOSPinsMain : public MWOSPins {

public:

    MWOSPinsMain() : MWOSPins() {
    }

    /**
     * Возвращает количество портов на текущей микросхемме
     */
    virtual uint8_t getCount() {
        return 128;
    }

    /**
     * Задать режим на вход или на выход
     * и подтяжку, если нужно
     * @param	outPort	настройить порт на выход (false - на вход)
     * @param   pull тип подтяжки порта (0-нет, 1-на 0, 2-на питание, 3 - на выход открытый коллектор, на вход - аналоговый)
     */
    virtual bool mode(bool outPort, uint8_t pull) {
        MW_LOG(F("mode: ")); MW_LOG(pin); MW_LOG('='); MW_LOG(outPort); MW_LOG(';'); MW_LOG_LN(outPort);
        if (outPort) { // на выход
            if (pull==mwos_pins_open_drain) {
#ifndef __AVR
                pinMode(pin, OUTPUT_OPEN_DRAIN); // открытый коллектор
#endif
            } else if (pull==mwos_pins_pull_down) {
#ifdef ESP32
                pinMode(pin, PULLDOWN); // включим подтяжку
#endif
            } else if (pull==mwos_pins_pull_up) {
#ifdef ESP32
                pinMode(pin, PULLUP); // включим подтяжку
#endif
            } else {
                pinMode(pin,OUTPUT);
            }
        } else { // на вход
            if (pull==mwos_pins_analog_input) {
#if defined(ESP32) || defined(STM32_MCU_SERIES)
#ifndef INPUT_ANALOG
#define INPUT_ANALOG ANALOG
#endif
                pinMode(pin,INPUT_ANALOG);
#else
                pinMode(pin,INPUT);
#endif
            } else
            if (pull==mwos_pins_pull_down) {
#if defined(ESP32) || defined(STM32_MCU_SERIES)
                pinMode(pin, INPUT_PULLDOWN);
#endif
#ifdef ESP8266
                if (pin==16) pinMode(pin, INPUT_PULLDOWN_16);
                else  pinMode(pin,INPUT);
#endif
            } else if (pull==mwos_pins_pull_up) {
                pinMode(pin, INPUT_PULLUP);

            } else pinMode(pin,INPUT);
        }
        return true;
    }

    virtual bool isPWM() {
#ifdef STM32_MCU_SERIES
        if (pin < 128) return true;
#endif
#ifdef __AVR_ATmega2560__
        return ((pin>1) && (pin<14));
#endif
#ifdef __AVR
        return ((pin==3) || (pin==5) || (pin==6) || (pin==9) || (pin==10) || (pin==11));
#endif
#ifdef ESP8266
        // один источник и любые GPIO https://esp8266.ru/forum/threads/shim.143/
#endif
#ifdef ESP32
        // несколько источников и любые GPIO
#endif
        return true;
    }

    virtual bool isADC() {
#if defined(__AVR_ATmega2560__)
        return ((pin>=A0) && (pin<=A15));
#endif
#if defined(__AVR)
        return ((pin>=A0) && (pin<=A7));
#endif
#ifdef STM32_MCU_SERIES
        return ((pin>=PA0) && (pin<=PA7)) || (pin==PB0) || (pin==PB1);
#endif
#ifdef ESP8266
        return (pin==A0);
#endif
#ifdef ESP32
        return (digitalPinToAnalogChannel(pin) >= 0);
#endif
        return false;
    }

    virtual bool writeDigital(bool dat) {
        MW_LOG(F("wd: ")); MW_LOG(pin); MW_LOG('='); MW_LOG_LN(dat);
        digitalWrite(pin,dat);
        return true;
    }

    virtual bool writeAnalog(uint16_t dat) {
        if (isPWM()) {
#ifdef STM32_MCU_SERIES
            analogWrite(pin,dat);
#endif
#if defined(__AVR)
            analogWrite(pin,dat);
#endif
            // TODO: тут делаем analogWrite для ESP32 и ESP8266
#if defined(ESP8266)
#endif
#if defined(ESP32)
            ledcAttachPin(pin, 0); // нулевой канал PWM
			ledcWrite(0,dat);
#endif
            return true;
        }
        return false;
    }

    virtual bool readDigital() {
        return digitalRead(pin);
    }

    virtual uint16_t readAnalog() {
        if (!isADC()) return readDigital();
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
#if defined(ESP32) || defined(ESP8266) || defined(STM32_MCU_SERIES)
    virtual bool attach(std::function<void(void)> intRoutine, int mode=RISING) {
        int8_t interrupt=getInterrupt();
        if (interrupt<0) return false;
#if defined(ESP8266)
        //attachInterrupt(interrupt,void (*)(void) intRoutine,mode);
#endif
#if defined(ESP32) || defined(ESP8266)
		attachInterrupt(interrupt,intRoutine,mode);
#endif
#ifdef STM32_MCU_SERIES
        //attachInterrupt(interrupt,(voidFuncPtr) intRoutine,mode);
#endif
        return true;
	}
#endif
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
