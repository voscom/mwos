#ifndef MWOS3_MWOSDisplayILI9488_H
#define MWOS3_MWOSDisplayILI9488_H

#include "SPI.h"
//#include <ILI9488.h>

/***
 * Модуль экрана для показа данных на дисплее ILI9488 (480x320)
 *

1 VCC power input (3.3V~5V)
2 GND power supply ground
3 CSLCD film selection
4 RESETLCD reset
5 DCLCD bus command/data selection
6 SDI (MoSi) LCD SPI display bus data input
7 SCKLCD SPI display bus clock signal
8 LEDLCD backlight control (high level lighting)
9 SDO (MISO) LCD SPI display bus data input and output.
10 TCK - SPI CLK t _ clk SPI bus clock signal of touch panel
11 TCS - SPI CS TOUCH Chip selection of SPI bus for 11 t _ cs touch panel
12 TDI - SPI MOSI t _ din touch panel SPI bus data input
13 TDO - SPI MISO t _ do touch panel SPI bus data output


 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/lovyan03/LovyanGFX.git
 *
 */
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <SPI.h>

class LGFX_ILI9488 : public lgfx::LGFX_Device {
    // lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX_ILI9488(MWOS_PIN_INT cs_pin, MWOS_PIN_INT dc_pin, MWOS_PIN_INT rst_pin,  MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi) {
        {
            auto cfg = _bus_instance.config();
            // SPI
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read =  16000000;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = 1;
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

            cfg.memory_width = 320;
            cfg.memory_height = 480;
            cfg.panel_width = 320;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;

            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

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
#define MWOS_COLOR_YELLOW  TFT_YELLOW
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

#define MWOSDisplayClass LGFX_ILI9488
#define MWOSDisplayFontClass GFXfont
#include "MWOSDisplay.h"

class MWOSDisplayILI9488 : public MWOSDisplay {
public:

#pragma pack(push,1)
    SPIClass * _spi;
#pragma pack(pop)

    MWOSDisplayILI9488() : MWOSDisplay() {
        name=(char *) F("displayILI9488");
    }

    MWOSDisplayILI9488(SPIClass *spi, MWOS_PIN_INT cs_pin, MWOS_PIN_INT dc_pin, MWOS_PIN_INT rst_pin, MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi) : MWOSDisplayILI9488() {
        _spi= spi;
        pin[0]=pin_sck;
        pin[1]=pin_miso;
        pin[2]=pin_mosi;
        pin[3]=cs_pin;
        pin[4]=dc_pin;
        pin[5]=rst_pin;
        // создадим дисплей
        display=new LGFX_ILI9488(cs_pin,dc_pin,rst_pin,pin_sck,pin_miso,pin_mosi);
    }

    /**
     * Первоначальная инициализация дисплея.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        bool res=((LGFX_ILI9488*) display)->init();
        MW_LOG_MODULE(this); MW_LOG(F("LGFX_ILI9488 onInit: ")); MW_LOG_LN(res);
        delay(5);
        return true;
    }

    virtual void sleep() { // выключить дисплей
        ((LGFX_ILI9488 *) display)->sleep();
        delay(5); // Задержка на время выключения перед отправкой другой команды
    }

    virtual void wake() { // включить дисплей
        ((LGFX_ILI9488 *) display)->wakeup();
        delay(120); // Задержка стабилизации питания
    }


};


#endif //MWOS3_MWOSDisplayILI9488_H
