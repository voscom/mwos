#ifndef MWOS3_MWOSPORTSMCP23017_H
#define MWOS3_MWOSPORTSMCP23017_H

#include "core/MWOSPins.h"
#include "core/iodev/MWStreamI2C.h"

#define MCP23017_ADDRESS 0x20

// registers
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14


#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255

/***
 * Блок портов микроконтроллера
 * Может находиться на внешней микросхемме
 */
class MWOSPinsMCP23017 : public MWOSPins {
protected:
    MWStreamI2C * i2c;
    uint16_t nowValue=0;

public:

    /***
     * Расширитель на 16 портов MCP23017
     * @param twoWire               Интерфейс i2c
     * @param devHardwareAddress    Аппаратный адрес, заданный ножками микросхеммы (от 0)
     * @param _firstPin Первый порт в адресном пространстве портов MWOS
     */
    MWOSPinsMCP23017(TwoWire * twoWire, uint8_t devHardwareAddress, int16_t _firstPin=0) : MWOSPins() {
        firstPin=_firstPin;
//        /i2c=new MWStreamI2C();
        i2c->begin(twoWire,MCP23017_ADDRESS+devHardwareAddress,I2C_8bit);
    }

    /**
     * Возвращает количество портов на текущей микросхемме
     */
    virtual uint8_t getCount() {
        return 16;
    }

    /**
     * Задать режим на вход или на выход
     * и подтяжку, если нужно
     * @param	outPort	настройить порт на выход (false - на вход)
     * @param   pull тип подтяжки порта (0-нет, 1-на 0, 2-на питание, 3 - на выход открытый коллектор, на вход - аналоговый)
     */
    virtual bool mode(bool outPort, uint8_t pull) {
        uint16_t outPorts=0;
        readRegister((uint8_t *) &outPorts,MCP23017_IODIRA); // читаем в outPorts текущее состояние
        writeRegister((uint8_t *) &outPorts,MCP23017_IODIRA,!outPort); // установим ввод-вывод
        uint16_t outPull=0;
        readRegister((uint8_t *) &outPull,MCP23017_GPPUA); // читаем в outPull текущее состояние
        writeRegister((uint8_t *) &outPull,MCP23017_GPPUA,pull==2); // установим подтяжку (только вверх)
        uint16_t outIPOL=0;
        readRegister((uint8_t *) &outIPOL,MCP23017_IPOLA); // читаем в outIPOL текущее состояние
        writeRegister((uint8_t *) &outIPOL,MCP23017_IPOLA,LOW);
        // закешируем текущие значения портов
        readRegister((uint8_t *) &nowValue,MCP23017_GPIOA);
        MW_LOG(F("MCP23017 mode pin:")); MW_LOG(pin); MW_LOG(';'); MW_LOG_LN(outPort);
        return true;
    }

    virtual bool isPWM() {
        return false;
    }

    virtual bool isADC() {
        return false;
    }

    virtual bool writeDigital(bool dat) {
        return writeRegister((uint8_t *) &nowValue,MCP23017_GPIOA,dat);
    }

    virtual bool readDigital() {
        return readRegister((uint8_t *) &nowValue,MCP23017_GPIOA);
    }

private:

    /***
     * Записать значение в регистровую пару
     * @param word  Слово (2 байта) с кешированными значениями
     * @param firstReg  Первый регистр от регистровой пары
     * @param dat   Значение для записи
     * @return  Регистр успешно записан
     */
    bool writeRegister(uint8_t * word, uint8_t firstReg, bool dat) {
        uint8_t pinLocal=pin-firstPin;
        uint8_t reg=firstReg;
        uint8_t i=0;
        if (pinLocal>7) {
            pinLocal-=8;
            i=1;
            reg++;
        }
        bitWrite(word[i],pinLocal,dat);
        if (i2c->writeByte(reg,word[i])>0) return true;
        return false;
    }

    /***
     * Считать значение регистровой пары
     * @param word  Слово (2 байта) для кеширования значений
     * @param firstReg  Первый регистр от регистровой пары
     * @return  Значение из регистра, для выбранного пина
     */
    bool readRegister(uint8_t * word, uint8_t firstReg) {
        uint8_t pinLocal=pin-firstPin;
        if (pinLocal>=getCount()) pinLocal=getCount()-1;
        uint8_t reg=firstReg;
        uint8_t i=0;
        if (pinLocal>7) {
            pinLocal-=8;
            i=1;
            reg++;
        }
        word[i]=i2c->readByte(reg);
        return bitRead(word[i],pinLocal);
    }


};


#endif //MWOS3_MWOSPORTSMCP23017_H
