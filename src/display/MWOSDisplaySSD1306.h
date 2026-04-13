#ifndef MWOS3_MWOSDISPLAYSSD1306_H
#define MWOS3_MWOSDISPLAYSSD1306_H
/***
 * Модуль экрана для показа данных на дисплее SSD1306
 *
 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/adafruit/Adafruit_SSD1306.git
 *
 */

#include "Wire.h"
#include "SPI.h"
#include <Adafruit_SSD1306.h>
#include "MWOSDisplay.h"

class MWOSDisplaySSD1306 : public MWOSDisplay {
public:

#pragma pack(push,1)
    TwoWire * _twi;
    SPIClass * _spi;
    uint8_t i2cAddr;
#pragma pack(pop)

    MWOSDisplaySSD1306() : MWOSDisplay() {
        name=(char *) F("displaySSD1306");
    }

    MWOSDisplaySSD1306(uint8_t w, uint8_t h, TwoWire *twi, MWOS_PIN_INT pin_sda, MWOS_PIN_INT pin_scl, MWOS_PIN_INT rst_pin = -1, uint8_t i2caddr=0x3C,  uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL) : MWOSDisplaySSD1306() {
        _twi= twi;
        i2cAddr=i2caddr;
        pin[0]=pin_sda;
        pin[1]=pin_scl;
        pin[2]=rst_pin;
        pin[3]=-1;
        pin[4]=-1;
        pin[5]=-1;
        display=new Adafruit_SSD1306(w,h,twi,rst_pin,clkDuring,clkAfter);
    }

    MWOSDisplaySSD1306(uint8_t w, uint8_t h, SPIClass *spi, MWOS_PIN_INT cs_pin, MWOS_PIN_INT dc_pin, MWOS_PIN_INT rst_pin, MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi, uint32_t bitrate = 8000000UL) : MWOSDisplaySSD1306() {
        _spi= spi;
        pin[0]=pin_sck;
        pin[1]=pin_miso;
        pin[2]=pin_mosi;
        pin[3]=cs_pin;
        pin[4]=dc_pin;
        pin[5]=rst_pin;
        MWOSDisplay::display=new Adafruit_SSD1306(w,h,spi,dc_pin,rst_pin,cs_pin,bitrate);
    }

    /**
     * Первоначальная инициализация дисплея.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        if (_twi!=NULL) return ((Adafruit_SSD1306 *) MWOSDisplay::display)->begin(SSD1306_SWITCHCAPVCC, i2cAddr);
        return true;
    }

    virtual void clear() { // очистить дисплей
        if (cmd==CMD_DONT_INITED) return;
        ((Adafruit_SSD1306 *) MWOSDisplay::display)->clearDisplay();
    }

    virtual void commit() { // применить изменения дисплея
        if (cmd==CMD_DONT_INITED) return;
        ((Adafruit_SSD1306 *) MWOSDisplay::display)->display();
    }


};


#endif //MWOS3_MWOSDISPLAYSSD1306_H
