#ifndef MWOS3_MWOSDISPLAYST7789V_H
#define MWOS3_MWOSDISPLAYST7789V_H

#include "Wire.h"
#include "SPI.h"
//#include <Adafruit_ST7789.h>
#include <LovyanGFX.hpp>

#ifndef MWOS_COLOR_WHITE
#ifdef ST77XX_WHITE
#define MWOS_COLOR_WHITE  ST77XX_WHITE
#else
#define MWOS_COLOR_WHITE  TFT_WHITE
#endif
#endif
#ifndef MWOS_COLOR_BLACK
#ifdef ST77XX_BLACK
#define MWOS_COLOR_BLACK  ST77XX_BLACK
#else
#define MWOS_COLOR_BLACK  TFT_BLACK
#endif
#endif
#ifndef MWOS_COLOR_GRAY
#define MWOS_COLOR_GRAY  0b1000010000010000
#endif
#ifndef MWOS_COLOR_GREEN
#ifdef ST77XX_GREEN
#define MWOS_COLOR_GREEN  ST77XX_GREEN
#else
#define MWOS_COLOR_GREEN  TFT_GREEN
#endif
#endif
#ifndef MWOS_COLOR_YELLOW
#ifdef ST77XX_YELLOW
#define MWOS_COLOR_YELLOW  ST77XX_YELLOW
#else
#define MWOS_COLOR_YELLOW  TFT_OLIVE
#endif
#endif
#ifndef MWOS_COLOR_RED
#ifdef ST77XX_RED
#define MWOS_COLOR_RED  ST77XX_RED
#else
#define MWOS_COLOR_RED  TFT_RED
#endif
#endif
#ifndef MWOS_COLOR_BLUE
#ifdef ST77XX_BLUE
#define MWOS_COLOR_BLUE  ST77XX_BLUE
#else
#define MWOS_COLOR_BLUE  TFT_BLUE
#endif
#endif

class LGFX_ST7789V : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX_ST7789V(uint16_t w, uint16_t h, MWOS_PIN_INT cs_pin, MWOS_PIN_INT dc_pin, MWOS_PIN_INT rst_pin,  MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi) {
        {
            auto cfg = _bus_instance.config();
            // SPI
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            //cfg.freq_read =  16000000;
            cfg.spi_3wire = true;
            //cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO; //1
            cfg.pin_sclk = pin_sck;
            cfg.pin_mosi = pin_mosi;
            cfg.pin_miso = pin_miso;
            cfg.pin_dc = dc_pin;

            // SD
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs = cs_pin;
            cfg.pin_rst = rst_pin;
            cfg.pin_busy = -1;

            cfg.panel_width      =   w;
            cfg.panel_height     =   h;

            cfg.invert           = true;

            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

#define MWOSDisplayClass LGFX_ST7789V
#define MWOSDisplayFontClass GFXfont
#include "MWOSDisplay.h"

/***
 * Модуль экрана для показа данных на дисплее ST7789V
 *
 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/lovyan03/LovyanGFX.git
 *
 */
class MWOSDisplayST7789V : public MWOSDisplay {
public:

#pragma pack(push,1)
    SPIClass * _spi;
    uint16_t _w, _h;
#pragma pack(pop)

    MWOSDisplayST7789V() : MWOSDisplay() {
        name=(char *) F("displayST7789V");
    }

    MWOSDisplayST7789V(uint16_t w, uint16_t h, SPIClass *spi, MWOS_PIN_INT cs_pin, MWOS_PIN_INT dc_pin, MWOS_PIN_INT rst_pin, MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi) : MWOSDisplayST7789V() {
        _spi= spi;
        pin[0]=pin_sck;
        pin[1]=pin_miso;
        pin[2]=pin_mosi;
        pin[3]=cs_pin;
        pin[4]=dc_pin;
        pin[5]=rst_pin;
        _w=w;
        _h=h;
        // создадим дисплей
        display=new LGFX_ST7789V(w, h, cs_pin, dc_pin, rst_pin, pin_sck, pin_miso, pin_mosi);
        //display=new LGFX_ST7789V(spi,cs_pin,dc_pin,rst_pin);
    }

    /**
     * Первоначальная инициализация дисплея.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        //((LGFX_ST7789V*) display)->init(_w, _h, SPI_MODE2);
        return ((MWOSDisplayClass*) display)->begin();
    }

    virtual void sleep() { // выключить дисплей
        //((LGFX_ST7789V *) display)->enableSleep(true);
        ((MWOSDisplayClass *) display)->sleep();
        delay(5); // Задержка на время выключения перед отправкой другой команды
    }

    virtual void wakeup() { // включить дисплей
        //((LGFX_ST7789V *) display)->enableSleep(false);
        ((MWOSDisplayClass *) display)->wakeup();
        delay(120); // Задержка стабилизации питания
    }

    virtual void commit() { // применить изменения дисплея
        if (cmd==CMD_DONT_INITED) return;
        ((MWOSDisplayClass *) display)->flush();
    }


};


#endif //MWOS3_MWOSDISPLAYST7789V_H
