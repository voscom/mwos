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
    MWOS_PIN_INT pin[3];
    uint8_t i2cAddr;
#pragma pack(pop)

    // занятые пины
    MWOS_PARAM(0, pins_i2c, mwos_param_uint32, mwos_param_readonly+mwos_param_pin, mwos_param_storage_no, 3);
    MWOS_PARAM(0, pins_spi, mwos_param_uint32, mwos_param_readonly+mwos_param_pin, mwos_param_storage_no, 7);

    MWOSDisplaySSD1306() : MWOSDisplay() {
        name=(char *) F("displaySSD1306");
    }

    MWOSDisplaySSD1306(uint8_t w, uint8_t h, TwoWire *twi, uint8_t i2caddr=0x3C, MWOS_PIN_INT rst_pin = -1, uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL) : MWOSDisplaySSD1306() {
        _twi= twi;
        i2cAddr=i2caddr;
        pin[0]=rst_pin;
        AddParam(&p_pins_i2c); // добавим пины i2c, что-бы было видно, что они заняты
        display=new Adafruit_SSD1306(w,h,twi,rst_pin,clkDuring,clkAfter);
    }

    MWOSDisplaySSD1306(uint8_t w, uint8_t h, SPIClass *spi, MWOS_PIN_INT dc_pin=-1, MWOS_PIN_INT rst_pin=-1, MWOS_PIN_INT cs_pin=-1, uint32_t bitrate = 8000000UL) : MWOSDisplaySSD1306() {
        _spi= spi;
        pin[0]=rst_pin;
        pin[1]=dc_pin;
        pin[2]=cs_pin;
        MWOSDisplay::AddParam(&p_pins_spi); // добавим пины SPI, что-бы было видно, что они заняты
        MWOSDisplay::display=new Adafruit_SSD1306(w,h,spi,dc_pin,rst_pin,cs_pin,bitrate);
    }


    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param==&p_pins_spi) {
            if (arrayIndex==0) return SCK;
            if (arrayIndex==1) return MISO;
            if (arrayIndex==2) return MOSI;
            if (arrayIndex==3) return SS;
            if (arrayIndex==4) return pin[0]; // rst
            if (arrayIndex==5) return pin[1]; // dc
            if (arrayIndex==6) return pin[2]; // cs
        }
        if (param==&p_pins_i2c) {
            if (arrayIndex==0) return SDA;
            if (arrayIndex==1) return SCL;
            if (arrayIndex==3) return pin[0]; // rst
        }
        return MWOSDisplay::getValue(param,arrayIndex);
    }

    virtual void onInit() {
        if(!((Adafruit_SSD1306 *) MWOSDisplay::display)->begin(SSD1306_SWITCHCAPVCC, i2cAddr)) {
            MWOSDisplay::display=NULL;
            MW_LOG_MODULE(this); MW_LOG_LN(F("Display SSD1306 allocation failed"));
            return;
        }
        MW_LOG_MODULE(this); MW_LOG_LN(F("Display SSD1306 onInit!"));
        MWOSDisplay::onInit();
        MWOSDisplay::SetDisplayUpdate();
    }

    virtual void clear() { // очистить дисплей
        ((Adafruit_SSD1306 *) MWOSDisplay::display)->clearDisplay();
    }

    virtual void commit() { // применить изменения дисплея
        ((Adafruit_SSD1306 *) MWOSDisplay::display)->display();
    }


};


#endif //MWOS3_MWOSDISPLAYSSD1306_H
