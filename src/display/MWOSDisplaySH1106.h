#ifndef MWOS3_MWOSDISPLAYSH1106_H
#define MWOS3_MWOSDISPLAYSH1106_H
/***
 * Модуль экрана для показа данных на дисплее SH1106
 *
 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/nhatuan84/esp32-sh1106-oled.git
 *
 */

#include "Wire.h"
#include "SPI.h"
#include "Adafruit_SH1106.h"
#include "MWOSDisplay.h"

class MWOSDisplaySH1106 : public MWOSDisplay {
public:

#pragma pack(push,1)
    TwoWire * _twi;
    SPIClass * _spi;
    MWOS_PIN_INT pin[3];
    uint8_t i2cAddr;
#pragma pack(pop)

    MWOSDisplaySH1106() : MWOSDisplay() {
        name=(char *) F("displaySH1106");
    }

    MWOSDisplaySH1106(uint16_t w, uint16_t h, TwoWire *twi, MWOS_PIN_INT pin_sda, MWOS_PIN_INT pin_scl, MWOS_PIN_INT rst_pin = -1, uint8_t i2caddr=0x3C, uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL) : MWOSDisplaySH1106() {
        _twi= twi;
        i2cAddr=i2caddr;
        pin[0]=pin_sda;
        pin[1]=pin_scl;
        pin[2]=rst_pin;
        pin[3]=-1;
        pin[4]=-1;
        pin[5]=-1;
        display=new Adafruit_SH1106();
    }

    MWOSDisplaySH1106(uint16_t w, uint16_t h, SPIClass *spi, int8_t cs_pin, int8_t dc_pin, int8_t rst_pin, MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi, uint32_t bitrate = 8000000UL) : MWOSDisplaySH1106() {
        _spi= spi;
        pin[0]=pin_sck;
        pin[1]=pin_miso;
        pin[2]=pin_mosi;
        pin[3]=cs_pin;
        pin[4]=dc_pin;
        pin[5]=rst_pin;
        MWOSDisplay::display=new Adafruit_SH1106(dc_pin,rst_pin,cs_pin);
    }

    /**
     * Первоначальная инициализация дисплея.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        if (_twi!=NULL) return ((Adafruit_SH1106 *) MWOSDisplay::display)->begin(SH1106_SWITCHCAPVCC, i2cAddr);
        return true;
    }

    virtual void clear() { // очистить дисплей
        ((Adafruit_SH1106 *) MWOSDisplay::display)->clearDisplay();
    }

    virtual void commit() { // применить изменения дисплея
        ((Adafruit_SH1106 *) MWOSDisplay::display)->display();
    }


};


#endif //MWOS3_MWOSDISPLAYSH1106_H
