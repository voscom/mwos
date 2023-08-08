#ifndef MWOS3_MWOSPINS_H
#define MWOS3_MWOSPINS_H
/***
 * Блок расширения портов микроконтроллера
 */
class MWOSPins {

public:

    /**
     * Первый порт на этом блоке (от него начинается нумерация портов на блоке)
     */
    MWOS_PIN_INT firstPin;
    MWOSPins * next=NULL; // Следующий модуль расширитель портов

    MWOSPins(MWOS_PIN_INT _firstPin=0) {
        firstPin=_firstPin;
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
            if (newPorts->firstPin<firstPin+getCount()) newPorts->firstPin=firstPin+getCount();
            MW_LOG(F("newPorts: ")); MW_LOG(newPorts->firstPin); MW_LOG('+'); MW_LOG_LN(newPorts->getCount());
            return newPorts;
        }
        return next->add(newPorts); // если следующая плата уже есть - рекурсивно вызовем для нее добавление
    }

    /**
     * Возвращает количество портов на текущем блоке
     */
    virtual uint8_t getCount() {
        return 128;
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
        return ((firstPin<=pin) && (firstPin+getCount()>pin));
    }

    /**
     * Задать режим на вход или на выход
     * и подтяжку, если нужно
     * @param	outPort	настройить порт на выход (false - на вход)
     * @param   pull тип подтяжки порта (0-нет, 1-на 0, 2-на питание, 3 - открытый коллектор)
     */
    virtual bool mode(bool outPort, uint8_t pull) {
        return true;
    }

    virtual bool isPWM() {
        return false;
    }

    virtual bool isDAC() {
        return false;
    }

    virtual bool isADC() {
        return false;
    }

    virtual bool writeDigital(bool dat) {
        return false;
    }

    virtual bool writeAnalog(uint16_t dat) {
        return false;
    }

    virtual bool readDigital() {
        return false;
    }

    virtual uint16_t readAnalog() {
        return 0;
    }

    virtual int8_t getInterrupt() {
        return -1;
    }

    /**
     * Подключить прерывание для порта
     */
#if defined(ESP32) || defined(ESP8266) || defined(STM32_MCU_SERIES)
    virtual bool attach(std::function<void(void)> intRoutine, int mode=RISING) {
		return false;
	}
#endif
    /**
     * Отключить прерывание для порта
     */
    virtual bool detach() {
        return false;
    }

protected:
    MWOS_PIN_INT pinReset=-1; // Порт, к которому подключена ножка RESET этого расширителя портов
    MWOS_PIN_INT pin; // Текущий порт, с которым идет работа
};



#endif //MWOS3_MWOSPINS_H
