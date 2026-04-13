#ifndef MWOS3_MWOSDisplayJC4827W543_H
#define MWOS3_MWOSDisplayJC4827W543_H


/***
 * Модуль экрана для показа данных на дисплее MWOSDisplayJC4827W543 (480x272)
 *

 * требует библиотеки: (нужно добавить в platform.ini)
    Arduino_GFX=https://github.com/moononournation/Arduino_GFX
    TouchLib=https://github.com/mmMicky/TouchLib
    lvgl/lvgl@^8.4.0
 *
 */
#include <Arduino_GFX_Library.h>

#define TFT_W 272
#define TFT_H 480
#define GFX_BL 1
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
Arduino_GFX *g = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 2 /* rotation */, true /* IPS */);
Arduino_GFX *gfx = new Arduino_Canvas(TFT_H /* width */, TFT_W /* height */, g);
#define CANVAS

#ifndef MWOS_COLOR_WHITE
#ifdef ST77XX_WHITE
#define MWOS_COLOR_WHITE  ST77XX_WHITE
#else
#define MWOS_COLOR_WHITE  WHITE
#endif
#endif
#ifndef MWOS_COLOR_BLACK
#ifdef ST77XX_BLACK
#define MWOS_COLOR_BLACK  ST77XX_BLACK
#else
#define MWOS_COLOR_BLACK  BLACK
#endif
#endif
#ifndef MWOS_COLOR_GRAY
#define MWOS_COLOR_GRAY  0b1000010000010000
#endif
#ifndef MWOS_COLOR_GREEN
#ifdef ST77XX_GREEN
#define MWOS_COLOR_GREEN  ST77XX_GREEN
#else
#define MWOS_COLOR_GREEN  GREEN
#endif
#endif
#ifndef MWOS_COLOR_YELLOW
#ifdef ST77XX_YELLOW
#define MWOS_COLOR_YELLOW  ST77XX_YELLOW
#else
#define MWOS_COLOR_YELLOW  YELLOW
#endif
#endif
#ifndef MWOS_COLOR_RED
#ifdef ST77XX_RED
#define MWOS_COLOR_RED  ST77XX_RED
#else
#define MWOS_COLOR_RED  RED
#endif
#endif
#ifndef MWOS_COLOR_BLUE
#ifdef ST77XX_BLUE
#define MWOS_COLOR_BLUE  ST77XX_BLUE
#else
#define MWOS_COLOR_BLUE  BLUE
#endif
#endif


#define MWOSDisplayClass Arduino_GFX
#define MWOSDisplayFontClass GFXfont
#include "MWOSDisplay.h"

class MWOSDisplayJC4827W543 : public MWOSDisplay {
public:

    MWOSDisplayJC4827W543() : MWOSDisplay() {
        name=(char *) F("displayJC4827W543");
        pin[0]=45;
        pin[1]=40;
        pin[2]=39;
        pin[3]=47;
        pin[4]=21;
        pin[5]=48;
#ifdef GFX_EXTRA_PRE_INIT
        GFX_EXTRA_PRE_INIT();
#endif
        display=gfx;
    }

    /**
     * Первоначальная инициализация дисплея.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        bool res=display->begin();
        MW_LOG_MODULE(this); MW_LOG(F("JC4827W543 onInit: ")); MW_LOG_LN(res);
        return res;
    }

    virtual void commit() { // применить изменения дисплея
        display->flush();
    }


};


#endif //MWOS3_MWOSDisplayILI9488_H
