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
    uint16_t firstPin;
    MWOSPins * next=NULL; // Следующий модуль расширитель портов

    MWOSPins() {
        firstPin=0;
        pinReset=255;
    }

    /***
     * Добавить плату расширения
      * @param newPorts Новый блок расширения портов
      * @return Объект блока расширения
      */
    MWOSPins *  add(MWOSPins * newPorts) {
        if (next==NULL) {
            next=newPorts;
            newPorts->firstPin=firstPin+getCount();
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
    void setResetPin(uint8_t resetPin=255) {
        pinReset=resetPin;
        if (pinReset<255) {
            pinMode(pinReset,OUTPUT);
            digitalWrite(pinReset,HIGH);
        }
    }

    /***
     * Сбросить этот блок расширения портов
     * @return
     */
    void reset() {
        if (pinReset<255) {
            digitalWrite(pinReset,LOW);
            delay(100);
            digitalWrite(pinReset,HIGH);
        }
    }

    bool isPin(uint16_t pinNum) {
        pin=pinNum;
        return ((firstPin<=pin) && (firstPin+getCount()>pin));
    }

    /**
     * Задать режим на вход или на выход
     * и подтяжку, если нужно
     * @param	pin	код порта
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
    uint8_t pinReset=255; // Порт, к которому подключена ножка RESET этого расширителя портов
    uint16_t pin; // Текущий порт, с которым идет работа
};



#endif //MWOS3_MWOSPINS_H
