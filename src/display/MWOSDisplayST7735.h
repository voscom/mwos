#ifndef MWOS3_MWOSDISPLAYST7735_H
#define MWOS3_MWOSDISPLAYST7735_H

#include "Wire.h"
#include "SPI.h"
#include <Adafruit_ST7735.h>

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


#include "MWOSDisplay.h"

/***
 * Модуль экрана для показа данных на дисплее ST7735
 *
 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/adafruit/Adafruit-ST7735-Library.git
 *
 */
class MWOSDisplayST7735 : public MWOSDisplay {
public:

#pragma pack(push,1)
    SPIClass * _spi;
#pragma pack(pop)

    MWOSDisplayST7735() : MWOSDisplay() {
        name=(char *) F("displayST7735");
        MWOSDisplay::AddParam(&p_pins_spi); // добавим пины SPI, что-бы было видно, что они заняты
    }

    MWOSDisplayST7735(uint16_t w, uint16_t h, SPIClass *spi, MWOS_PIN_INT cs_pin, MWOS_PIN_INT dc_pin, MWOS_PIN_INT rst_pin,  MWOS_PIN_INT pin_sck, MWOS_PIN_INT pin_miso, MWOS_PIN_INT pin_mosi) : MWOSDisplayST7735() {
        _spi= spi;
        pin[0]=pin_sck;
        pin[1]=pin_miso;
        pin[2]=pin_mosi;
        pin[3]=cs_pin;
        pin[4]=dc_pin;
        pin[5]=rst_pin;
        MWOSDisplay::display=new Adafruit_ST7735(spi,cs_pin,dc_pin,rst_pin);
    }

    /**
     * Первоначальная инициализация дисплея.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        ((Adafruit_ST7735*) MWOSDisplay::display)->initR(INITR_BLACKTAB);
        delay(5);
        return true;
    }

    virtual void sleep() { // выключить дисплей
        ((Adafruit_ST7735 *) MWOSDisplay::display)->enableSleep(true);
        delay(5); // Задержка на время выключения перед отправкой другой команды
    }

    virtual void wake() { // включить дисплей
        ((Adafruit_ST7735 *) MWOSDisplay::display)->enableSleep(false);
        delay(120); // Задержка стабилизации питания
    }

    virtual void commit() { // применить изменения дисплея
        if (cmd==CMD_DONT_INITED) return;
        ((Adafruit_ST7735 *) MWOSDisplay::display)->flush();
    }

};


#endif //MWOS3_MWOSDISPLAYST7735_H
