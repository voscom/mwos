#ifndef MWOS3_MWOSPORTSMCP4725_H
#define MWOS3_MWOSPORTSMCP4725_H

#include "core/MWOSPins.h"
#include "core/iodev/MWStreamI2C.h"

#define MCP4725_MAXVALUE        4095

/***
 * Порт ЦАП на микросхемме MCP4725 по i2c
 */
class MWOSPinsMCP4725 : public MWOSPins {
protected:
    TwoWire * _wire;
    uint16_t nowValue=0;
    uint8_t _addr;

public:

    /***
     * Расширитель на 1 ЦАП-порт MCP4725
     * @param twoWire               Интерфейс i2c
     * @param devHardwareAddress    Аппаратный адрес, заданный ножками микросхеммы (от 0)
     * @param _firstPin Порт в адресном пространстве портов MWOS
     */
    MWOSPinsMCP4725(TwoWire * twoWire, uint8_t devHardwareAddress=0, int16_t _firstPin=4725) : MWOSPins() {
        firstPin=_firstPin;
        lastPin=_firstPin;
        _addr=devHardwareAddress+0x60;
        _wire=twoWire;
    }

    /**
     * Задать режим порта (вход, выход, подтяжка...)
     * @param pinMode   Режим порта
     * @return Успешно, или нет
     */
    virtual bool mode(MW_PIN_MODE mode) {
        if ((_addr < 0x60) || (_addr > 0x67) || !isConnected()) return false;
        return true;
    }

    virtual bool writeDAC(uint16_t dat) {
        if (dat == nowValue) return true;
        if (dat > MCP4725_MAXVALUE) dat=MCP4725_MAXVALUE;
        int rv = writeFastMode(dat);
        if (rv == 0) nowValue = dat;
        return (rv == 0);
    }

    virtual bool write(bool dat) {
        if (dat) return writeDAC(MCP4725_MAXVALUE);
        return writeDAC(0);
    }

private:

    int writeFastMode(const uint16_t value) {
        uint8_t l = value & 0xFF;
        uint8_t h = ((value / 256) & 0x0F);  //  set C0 = C1 = 0, no PDmode
        uint8_t  _powerDownMode=0;
        h = h | (_powerDownMode << 4);

        _wire->beginTransmission(_addr);
        _wire->write(h);
        _wire->write(l);
        return _wire->endTransmission();
    }

    bool isConnected() {
        _wire->beginTransmission(_addr);
        return (_wire->endTransmission() == 0);
    }

};


#endif //MWOS3_MWOSPORTSMCP23017_H
