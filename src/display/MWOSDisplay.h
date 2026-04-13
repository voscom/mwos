#ifndef MWOS3_MWOSDISPLAY_H
#define MWOS3_MWOSDISPLAY_H
/***
 * Модуль экрана для показа данных на дисплее
 * Позволяет показывать виджеты на дисплей, совместимый с Adafruit-GFX-Library или с похожими классами
 * Это не самостоятельный класс - дисплеи должны быть унаследованы от него
 * пример: MWOSDisplaySSD1306.h
 *
 * после вызова метода SetDisplayUpdate:
 * 1. очищает дисплей
 * 2. ждет перерисовки всех виджетов методом виджета print()
 * 3. на следующий кадр применяет изменения на дисплей
 *
 * Недостаток этого метода в том, что при каждом изменении любого виджета
 * - перерисовывает все виджеты, что может занять некоторое время
 * зато удобно и универсально!
 *
 *
 */

#include <SPI.h>
#include <Wire.h>
#include <core/MWOSModule.h>

#ifndef MWOS_COLOR_WHITE
#define MWOS_COLOR_WHITE 1
#endif
#ifndef MWOS_COLOR_BLACK
#define MWOS_COLOR_BLACK 0
#endif
#ifndef MWOS_COLOR_WHITE
#define MWOS_COLOR_WHITE 1
#endif
#ifndef MWOS_COLOR_MAIN
#define MWOS_COLOR_MAIN MWOS_COLOR_WHITE
#endif
#ifndef MWOS_COLOR_BACK
#define MWOS_COLOR_BACK MWOS_COLOR_BLACK
#endif
#ifndef MWOS_COLOR_GRAY
#define MWOS_COLOR_GRAY MWOS_COLOR_MAIN
#endif
#ifndef MWOS_COLOR_RED
#define MWOS_COLOR_RED MWOS_COLOR_MAIN
#endif
#ifndef MWOS_COLOR_GREEN
#define MWOS_COLOR_GREEN MWOS_COLOR_MAIN
#endif
#ifndef MWOS_COLOR_YELLOW
#define MWOS_COLOR_YELLOW MWOS_COLOR_MAIN
#endif

// показывать на всех экранах
#define MWOS_SCREEN_ALL UINT16_MAX

#ifndef MWOSDisplayClass
#define MWOSDisplayFontClass GFXfont
#define MWOSDisplayClass Adafruit_GFX
#define ADAFRUIT_GFX
#endif

#ifndef MW_KEY_UP
#define MW_KEY_UP 'U'
#endif
#ifndef MW_KEY_DOWN
#define MW_KEY_DOWN 'D'
#endif
#ifndef MW_KEY_OK
#define MW_KEY_OK 'E'
#endif
#ifndef MW_KEY_LEFT
#define MW_KEY_LEFT 'L'
#endif
#ifndef MW_KEY_RIGHT
#define MW_KEY_RIGHT 'R'
#endif
#ifndef MW_KEY_EXIT
#define MW_KEY_EXIT 'X'
#endif
#ifndef MW_KEY_ESC
#define MW_KEY_ESC 'S'
#endif

// типы юнита
enum DisplayCmd : uint8_t {
    CMD_NO_ACTION = 0, // ничего не делать
    CMD_NEED_UPDATE = 1, // необходимо отрисовать все виджеты методом print
    CMD_UPDATE_NOW = 2, // необходимо применить вывод нового изображения для дисплея
    CMD_DONT_INITED = 255 // признак, что дисплей не проинициализирован
};

/**
 * Выравнивание виджета
 */
enum MWOS_WIDGET_ALIGN: uint8_t {
    UpLeft=0,
    UpCenter=1,
    UpRight=2,
    CenterLeft=3,
    CenterCenter=4,
    CenterRight=5,
    DownLeft=6,
    DownCenter=7,
    DownRight=8
};

class MWOSDisplay : public MWOSModule {
public:
    MWOSDisplayClass * display;
#pragma pack(push,1)
    DisplayCmd cmd=CMD_DONT_INITED;
    uint16_t vscreen=0; // текущий виртуальный экран (показывает только виджеты этого виртуального экрана)
    MWOS_PIN_INT pin[6];
    mwos_color _backColor=0;
    uint8_t _rotate=0;
    uint16_t _margin[4]={0,0,0,0}; // отступы от границ экрана (0-верх,1-низ,2-лево,3-право)
#pragma pack(pop)

    // номер текущего виртуального экрана (показывает только виджеты этого виртуального экрана)
    MWOS_PARAM(0, vscreen, mwos_param_uint16, mwos_param_control, MWOS_STORAGE_RTC, 1);

    // таймаут через который проверяется изменения значений (dSec [сек/10])
    MWOS_PARAM(1, size, mwos_param_uint16, mwos_param_option+mwos_param_readonly, MWOS_STORAGE_NO, 2);

    // занятые пины (задается в наследниках)
    MWOS_PARAM(2, pin, MWOS_PIN_INT_PTYPE, mwos_param_readonly+mwos_param_pin, MWOS_STORAGE_NO, 6);

    // фон экрана
    MWOS_PARAM(3, backColor, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    // поворот экрана
    MWOS_PARAM_F(4, rotate, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1,
                 "{'name':'rotate','value_format':'0`;90`;180`;270`;Перевернуть'}");
    // отступы от границ экрана (0-верх,1-низ,2-лево,3-право)
    MWOS_PARAM(5, margin, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 4);

    MWOSDisplay() : MWOSModule((char *) F("display")) {
        moduleType=MODULE_DISPLAY;
        AddParam(&p_vscreen);
        AddParam(&p_size);
        AddParam(&p_pin);
        AddParam(&p_backColor);
        AddParam(&p_rotate);
        AddParam(&p_margin);
    }

    /**
     * Можно непосредственно задавать дисплей, не требующий комитов. Дисплей уже должен быть инициализован.
     * Но лучше использовать потомков (пример - MWOSDisplaySSD1306.h)
     * @param _display
     */
    MWOSDisplay(MWOSDisplayClass * _display) : MWOSDisplay() {
        display=_display;
    }

    /**
     * Задать необходимость обновления дисплея
     * на следующий такт дисплей будет очищен, а все его виджеты - обновлены
     */
    void SetDisplayUpdate() {
        cmd=CMD_NEED_UPDATE;
    }

    /**
     * Задать новый виртуальный экран
     * @param newScreen
     */
    void SetVScreen(uint16_t newScreen) {
        vscreen=newScreen;
        SetDisplayUpdate();
    }

    bool IsNeedUpdated() {
        return cmd==CMD_UPDATE_NOW;
    }

    /**
     * Первоначальная инициализация дисплея.
     * Вызывается автоматически.
     * В потомках необходимо провести первоначальную инициализацию дисплея
     * @return  Успешно
     */
    virtual bool onInit() {
        return false;
    }

    /**
     * Первоначальная инициализация дисплея.
     * Вызывается вручную, если нужно выводить на дисплей до старта MWOS (заставка).
     */
    bool init() {
        if (cmd==CMD_DONT_INITED) {
            if (!onInit()) return false;
            cmd = CMD_NEED_UPDATE;
            delay(20);
            display->setRotation(_rotate);
            clear();
        }
        return true;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            uint16_t old_vscreen = vscreen;
            vscreen = MWOSModule::loadValue(vscreen, &p_vscreen, 0);
            _backColor = MWOSModule::loadValue(_backColor, &p_backColor, 0);
            _margin[0] = MWOSModule::loadValue(_margin[0], &p_margin, 0);
            _margin[1] = MWOSModule::loadValue(_margin[1], &p_margin, 1);
            _margin[2] = MWOSModule::loadValue(_margin[2], &p_margin, 2);
            _margin[3] = MWOSModule::loadValue(_margin[3], &p_margin, 3);
            _rotate = MWOSModule::loadValue(_rotate, &p_rotate, 0);
            if (modeEvent==EVENT_INIT) init();
            if (cmd == CMD_DONT_INITED) return;
            if (old_vscreen != vscreen) cmd = CMD_NEED_UPDATE;
            if (display->getRotation()!=_rotate) {
                display->setRotation(_rotate);
            }
            commit();
            //cmd = CMD_NEED_UPDATE;
            cmd = CMD_NO_ACTION;
            MWOSDisplay::SetDisplayUpdate();

        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (cmd==CMD_DONT_INITED) return;
            if (cmd==CMD_UPDATE_NOW) {
                cmd=CMD_NO_ACTION;
                commit();
            } else
            if (cmd==CMD_NEED_UPDATE) {
                clear();
                cmd=CMD_UPDATE_NOW;
            }
        } else
        if (modeEvent==EVENT_POWER_OFF) { // Вызывается перед выключением
            sleep();
        }
    }

    virtual void sleep() { // выключить дисплей
        //display->writeCommand(0x10); // Sleep
        //delay(5); // Задержка на время выключения перед отправкой другой команды
    }

    virtual void wake() { // включить дисплей
        //display->writeCommand(0x11); // WAKEUP
        //delay(120); // Задержка стабилизации питания
    }

    virtual void clear() { // очистить дисплей
        if (cmd==CMD_DONT_INITED) return;
        display->fillScreen(_backColor);
    }

    virtual void commit() { // применить изменения дисплея
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param==&p_size) {
            if (arrayIndex==0) return display->width();
            if (arrayIndex==1) return display->height();
        }
        if (param==&p_pin) {
            if (arrayIndex>=6) return 0;
            return pin[arrayIndex];
        }
        if (param==&p_vscreen) return vscreen;
        if (param==&p_rotate) return _rotate;
        if (param==&p_backColor) return _backColor;
        if (param==&p_margin) return _margin[arrayIndex];
        return MWOSModule::getValue(param,arrayIndex);
    }

    void setDefault(uint8_t rotate, mwos_color backColor) {
        _backColor=backColor;
        _rotate=rotate;
    }

    /**
     * Задать отступы экрана по умолчанию
     * @param marginUp
     * @param marginDown
     * @param marginLeft
     * @param marginRight
     */
    void setDefaultMargin(uint16_t marginUp,uint16_t marginDown,uint16_t marginLeft,uint16_t marginRight ) {
        _margin[0]=marginUp;
        _margin[1]=marginDown;
        _margin[2]=marginLeft;
        _margin[3]=marginRight;
    }

    uint16_t width() {
        if (cmd==CMD_DONT_INITED) {
#if defined(TFT_H) && defined(TFT_W)
            if (_rotate==0 || _rotate==2) return TFT_W-_margin[2]-_margin[3];
            return TFT_H-_margin[2]-_margin[3];
#endif
        }
        return display->width()-_margin[2]-_margin[3];
    }

    uint16_t height() {
        if (cmd==CMD_DONT_INITED) {
#if defined(TFT_H) && defined(TFT_W)
            if (_rotate==0 || _rotate==2) return TFT_H-_margin[0]-_margin[1];
            return TFT_W-_margin[0]-_margin[1];
#endif
        }
        return display->height()-_margin[0]-_margin[1];
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,mwos_color color) {
        //MW_LOG_MODULE(this); MW_LOG(F("fillRect ")); MW_LOG(x+_margin[2]); MW_LOG(':'); MW_LOG(y+_margin[0]); MW_LOG(';');  MW_LOG(w); MW_LOG('x'); MW_LOG_LN(h);
        display->fillRect(x+_margin[2],y+_margin[0],w,h,color);
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,mwos_color color) {
        display->drawRect(x+_margin[2],y+_margin[0],w,h,color);
    }

    /**
     * Нарисовать треугольную стрелочку в задвнном направлении
     * @param x     Верхний-левый угол прямоугольника с вписанной стрелкой
     * @param y     Верхний-левый угол прямоугольника с вписанной стрелкой
     * @param w     Ширина прямоугольника с вписанной стрелкой
     * @param h     Высота прямоугольника с вписанной стрелкой
     * @param align Направление стрелки: UpCenter,DownCenter,CenterLeft,CenterRight
     * @param color
     */
    void fillTriangleArrow(int16_t x, int16_t y, int16_t w, int16_t h, MWOS_WIDGET_ALIGN align, mwos_color color) {
        int16_t x0=x+_margin[2];
        int16_t y0=y+_margin[0];
        int16_t x1,x2,y1,y2;
        switch (align) {
            case UpCenter: {
                x1=x0+w/2;
                y1=y0;
                x2=x0+w;
                y2=y0+h;
                x0=x0;
                y0=y0+h;
            } break;
            case DownCenter: {
                x1=x0+w/2;
                y1=y0+h;
                x2=x0+w;
                y2=y0;
            } break;
            case CenterLeft: {
                x1=x0;
                y1=y0+h/2;
                x2=x0+w;
                y2=y0+h;
                x0=x0+w;
                y0=y0;
            } break;
            default: { // иначе всегда CenterRight
                x1=x0+w;
                y1=y0+h/2;
                x2=x0;
                y2=y0+h;
            }
        }
        display->fillTriangle(x0,y0,x1,y1,x2,y2, color);
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, mwos_color color) {
        display->drawBitmap(x+_margin[2],y+_margin[0],bitmap,w,h,color);
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, mwos_color color) {
        display->drawLine(x0+_margin[2],y0+_margin[0],x1+_margin[2],y1+_margin[0],color);
    }

    void setFont(const uint8_t *f) {
        display->setFont((MWOSDisplayFontClass *)f);
    }

    void setCursor(int16_t x, int16_t y) {
        //MW_LOG_MODULE(this); MW_LOG(F("setCursor ")); MW_LOG(x+_margin[2]); MW_LOG(':'); MW_LOG_LN(y+_margin[0]);
        display->setCursor(x+_margin[2],y+_margin[0]);
    }

    size_t print(const String &str) {
        return display->print(str);
    }

    size_t print(char ch) {
        return display->print(ch);
    }

    void setTextSize(uint8_t size) {
        display->setTextSize(size);
    }

    void setTextColor(uint16_t color) {
        display->setTextColor(color);
    }

    void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
#if defined(ADAFRUIT_GFX) || defined(_ARDUINO_GFX_LIBRARIES_H_)
        display->getTextBounds(str,x,y,x1,y1,w,h);
#else
        *x1=x;
        *y1=y;
        *w=display->textWidth(str);
        *h=display->fontHeight();
#endif
    }


};


#endif //MWOS3_MWOSDISPLAY_H
