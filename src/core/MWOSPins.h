#ifndef MWOS3_MWOSPINS_H
#define MWOS3_MWOSPINS_H

#include <stdlib.h>
#ifdef _GLIBCXX_STD_FUNCTION_H
#define voidFuncPtr std::function<void(void)>
#else
typedef void (*voidFuncPtr)(void);
voidFuncPtr _callback=NULL;
#endif

enum MW_PIN_MODE: uint8_t {
    MW_PIN_INPUT =0,
    MW_PIN_INPUT_PULLDOWN =1,
    MW_PIN_INPUT_PULLUP =2,
    MW_PINT_INPUT_ADC=3,
    MW_PIN_OUTPUT =8,
    MW_PIN_OUTPUT_OPEN_DRAIN =9,
    MW_PIN_OUTPUT_PWM =10,
    MW_PIN_OUTPUT_DAC =11,

    MW_PIN_DONT_INIT = 255
};


/***
 * Блок расширения портов микроконтроллера
 */
class MWOSPins {
public:

    /**
     * Первый порт на этом блоке (от него начинается нумерация портов на блоке)
     */
    MWOS_PIN_INT firstPin;
    MWOS_PIN_INT lastPin;
    MWOSPins * next=NULL; // Следующий модуль расширитель портов

    MWOSPins(MWOS_PIN_INT _firstPin=0) {
        firstPin=_firstPin;
        lastPin=127;
        pinReset=-1;
    }

    /***
     * Добавить плату расширения
      * @param newPorts Новый блок расширения портов
      * @return Объект блока расширения
      */
    MWOSPins *  add(MWOSPins * newPorts) {
        if (next==NULL) {
            next=newPorts;
            if (newPorts->firstPin<=lastPin) newPorts->firstPin=lastPin+1;
            MW_LOG(F("newPorts: ")); MW_LOG(newPorts->firstPin); MW_LOG(':'); MW_LOG_LN(newPorts->lastPin);
            return newPorts;
        }
        return next->add(newPorts); // если следующая плата уже есть - рекурсивно вызовем для нее добавление
    }

    /**
     * Задать ножку для сброса этого блока расширения портов
     * @param resetPin Порт ножки микроконтроллера, куда подключен RESET этого блока расширителя портов (по умолчанию - не задан)
     * @return
     */
    void setResetPin(MWOS_PIN_INT resetPin=-1) {
        pinReset=resetPin;
        if (pinReset>=0) {
            pinMode(pinReset,OUTPUT);
            digitalWrite(pinReset,HIGH);
        }
    }

    /***
     * Сбросить этот блок расширения портов
     * @return
     */
    void reset() {
        if (pinReset>=0) {
            digitalWrite(pinReset,LOW);
            delay(100);
            digitalWrite(pinReset,HIGH);
        }
    }

    bool isPin(MWOS_PIN_INT pinNum) {
        pin=pinNum;
        return ((firstPin<=pin) && (lastPin>=pin));
    }

    /**
     * Задать режим порта (вход, выход, подтяжка...)
     * @param pinMode   Режим порта
     * @return Успешно, или нет
     */
    virtual bool mode(MW_PIN_MODE mode) {
        return false;
    }

    virtual bool write(bool dat) {
        return false;
    }

    virtual bool writePWM(uint16_t dat) {
        return false;
    }

    virtual bool writeDAC(uint16_t dat) {
        return false;
    }

    virtual bool read() {
        return false;
    }

    virtual uint32_t readADC() {
        return false;
    }

    virtual int8_t getInterrupt() {
        return -1;
    }

    /**
     * Подключить прерывание для порта
     */
    virtual bool attach(voidFuncPtr funcPtr, int mode) {
        return false;
    }

    /**
     * Отключить прерывание для порта
     */
    virtual bool detach() {
        return false;
    }

protected:
    MWOS_PIN_INT pinReset=-1; // Порт, к которому подключена ножка RESET этого расширителя портов
    MWOS_PIN_INT pin; // Текущий порт, с которым идет работа

    /**
     * Есть ли этот пин в массиве?
     * @param arrayPins Массив
     * @param arraySize Размер массива
     * @return  Есть или нет
     */
    bool IsPinInArray(uint8_t * arrayPins, uint8_t arraySize) {
        if (pin<0 || arraySize<1 || arrayPins==NULL) return false;
        for (size_t i = 0; i < arraySize; i++) {
            uint8_t pn = pgm_read_byte_near(arrayPins + i);
            if (pn==pin) return true;
        }
        return false;
    }

};



#endif //MWOS3_MWOSPINS_H
