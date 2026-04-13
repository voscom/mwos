#ifndef MWOS3_ESP32S3_MWOSWIDGET_H
#define MWOS3_ESP32S3_MWOSWIDGET_H

#include "display/MWOSDisplay.h"
#include "core/MWOSConsts.h"

/**
 * Виджет дисплея MWOS
 * Показывает виджет на дисплее MWOSDisplay
 * Это не самостоятельный класс - виждеты должны быть унаследованы от него
 *
 * дочерний виджет должен обеспечивать отрисовку своего текущего состояния по методу print()
 * а такт же при изменении состояния дочерний виджет должен вызывать метод дисплея displayModule->SetDisplayUpdate()
 *
 *
 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/adafruit/Adafruit-GFX-Library.git
*/
class MWOSWidget : public MWOSModule {
public:


#pragma pack(push,1)
    MWOSDisplay * displayModule;
    int8_t _visible =0;
    int8_t _needUpdate =0;
    MWOS_WIDGET_ALIGN _align=UpLeft;
    int8_t _displayUpdate= 0;
    int16_t _x =0;
    int16_t _y =0;
    int16_t _width =2000;
    int16_t _height =16;
    mwos_color _color =MWOS_COLOR_WHITE;
    /**
     * номер виртуального экрана этого виджета (показывает только виджеты этого виртуального экрана).
     * если 0xffff - показывает на всех экранах
     */
    uint16_t vscreen=0;
#pragma pack(pop)

    // номер виртуального экрана этого виджета (показывает только виджеты этого виртуального экрана)
    MWOS_PARAM(0, vscreen, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // видимость виджета
    MWOS_PARAM(1, visible, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // выравнивание виджета MWOS_WIDGET_ALIGN
    MWOS_PARAM_F(2, align, mwos_param_bits4, mwos_param_option, MWOS_STORAGE_EEPROM, 1,
                 "{'name':'align','value_format':'UpLeft;UpCenter;UpRight;CenterLeft;CenterCenter;CenterRight;DownLeft;DownCenter;DownRight'}");
    // перерисовка виджета вызывает перерисовку всего дисплея (или только области виджета)
    MWOS_PARAM(3, displayUpdate, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // координата x
    MWOS_PARAM(4, x, mwos_param_int16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // координата y
    MWOS_PARAM(5, y, mwos_param_int16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // размер по X
    MWOS_PARAM(6, w, mwos_param_int16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // размер по Y
    MWOS_PARAM(7, h, mwos_param_int16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // цвет текста
    MWOS_PARAM(8, color, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    MWOSWidget(MWOSDisplay * _displayModule, char * unit_name) : MWOSModule(unit_name) {
        moduleType=MODULE_WIDGET;
        displayModule=_displayModule;
        AddParam(&p_vscreen);
        AddParam(&p_visible);
        AddParam(&p_align);
        AddParam(&p_displayUpdate);
        AddParam(&p_x);
        AddParam(&p_y);
        AddParam(&p_w);
        AddParam(&p_h);
        AddParam(&p_color);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            if (displayModule->cmd==CMD_DONT_INITED) {
                _visible= 0;
            } else {
                _visible = MWOSWidget::loadValue(_visible, &p_visible, 0);
            }
            _align=(MWOS_WIDGET_ALIGN) MWOSWidget::loadValue(_align, &p_align, 0);
            _displayUpdate=MWOSWidget::loadValue(_displayUpdate, &p_displayUpdate, 0);
            vscreen=MWOSWidget::loadValue(vscreen, &p_vscreen, 0);
            int32_t w=displayModule->width();
            int32_t h=displayModule->height();
            /*
            if ((displayModule->_rotate & 1) >0) { // экран повернут боком
                w=displayModule->display->height();
                h=displayModule->display->width();
            } else {  // экран не повернут на бок
                w=displayModule->display->width();
                h=displayModule->display->height();
            } */
            // размеры и координаты виджета не должны быть больше размера экрана и могут задаваться в процентах от экрана (отрицательным числом)
            if (_width<0) _width=((int32_t ) ((int32_t ) (-_width) * w))/100; // отрицательное значение - процент от размера экрана
            if (_width>w) _width=w;
            if (_height<0) _height=((int32_t ) ((int32_t ) (-_height) *h))/100; // отрицательное значение - процент от размера экрана
            if (_height>h) _height=h;
            _width=MWOSWidget::loadValue(_width, &p_w, 0);
            _height=MWOSWidget::loadValue(_height, &p_h, 0);
            if (_width<0) _width=((int32_t ) ((int32_t ) (-_width) * w))/100; // отрицательное значение - процент от размера экрана
            if (_width>w) _width=w;
            if (_height<0) _height=((int32_t ) ((int32_t ) (-_height) *h))/100; // отрицательное значение - процент от размера экрана
            if (_height>h) _height=h;
            // виджет не должен выходить за пределы экрана
            if (_x<0) _x=((int32_t ) ((int32_t ) (-_x) * w))/100; // отрицательное значение - процент от размера экрана
            if (_x+_width>w) _x=w-_width;
            if (_y<0) _y=((int32_t ) ((int32_t ) (-_y) * h))/100; // отрицательное значение - процент от размера экрана
            if (_y+_height>h) _y=h-_height;
            _x=MWOSWidget::loadValue(_x, &p_x, 0);
            _y=MWOSWidget::loadValue(_y, &p_y, 0);
            if (_x<0) _x=((int32_t ) ((int32_t ) (-_x) * w))/100; // отрицательное значение - процент от размера экрана
            if (_x+_width>w) _x=w-_width;
            if (_y<0) _y=((int32_t ) ((int32_t ) (-_y) * h))/100; // отрицательное значение - процент от размера экрана
            if (_y+_height>h) _y=h-_height;
            _color=MWOSWidget::loadValue(_color, &p_color, 0);
            MW_LOG_MODULE(this); MW_LOG(F("onInit: v")); MW_LOG(_visible); MW_LOG('x'); MW_LOG(_x); MW_LOG(';'); MW_LOG(_y); MW_LOG('s'); MW_LOG(_width); MW_LOG(';'); MW_LOG(_height); MW_LOG('c'); MW_LOG_LN(_color);
            if (vscreen==UINT16_MAX || vscreen==displayModule->vscreen) displayModule->SetDisplayUpdate();
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (IsVisible() && (IsNeedUpdated() || displayModule->IsNeedUpdated())) {
                print();
            }
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 0: return vscreen;
            case 1: return _visible;
            case 2: return _align;
            case 3: return _displayUpdate;
            case 4: return _x;
            case 5: return _y;
            case 6: return _width;
            case 7: return _height;
            case 8: return _color;
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        _needUpdate=false;
    }

    /***
     * Задать параметры по умолчанию для этого виджета (эти параметры могут быть изменены удаленно)
     * @param x     Координата X на дисплее
     * @param y     Координата Y на дисплее
     * @param width Размер по X
     * @param height Размер по Y
     * @param color Цвет
     * @param numVScreen Номер виртуального экрана
     */
    void setDefault(bool visible, int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color, uint16_t numVScreen=MWOS_SCREEN_ALL, MWOS_WIDGET_ALIGN align=UpLeft) {
        _visible=visible;
        _x=x;
        _y=y;
        _width=width;
        _height=height;
        _color=color;
        vscreen=numVScreen;
        _align=align;
    }

    int16_t alignX(int16_t x, int16_t rectWidth, int16_t nowWidth) {
        if (rectWidth>displayModule->width()) rectWidth=displayModule->width();
        int16_t dW=rectWidth-nowWidth;
        if (dW!=0) { // выравнивание по горизонтали
            uint8_t _alignX=_align % 3;
            if (_alignX == 1) x=x+dW/2;
            else if (_alignX == 2) x=x+dW-1;
        }
        return x;
    }

    int16_t alignY(int16_t y, int16_t rectHeight, int16_t nowHeight) {
        if (rectHeight>displayModule->height()) rectHeight=displayModule->height();
        int16_t dH=rectHeight-nowHeight;
        if (dH!=0) { // выравнивание по горизонтали
            uint8_t _alignY=_align / 3;
            if (_alignY == 1) y=y+dH/2;
            else if (_alignY == 2) y=y+dH-1;
        }
        return y;
    }

    /**
     * Этот элемент видно
     * @return
     */
    bool IsVisible() {
        return _visible>0 && (vscreen==UINT16_MAX || displayModule->vscreen==vscreen);
    }

    void show() {
        _visible= true;
        _needUpdate = true;
    }

    void hide() {
        if (_visible) _needUpdate = true;
        _visible= false;
    }

    /**
     * Этот элемент видно
     * @return
     */
    bool IsNeedUpdated() {
        return _needUpdate;
    }

    /**
     * Сделать этот элемент видимым и его виртуальный экран - основным
     */
    void SetVisible() {
        _visible=1;
        if (vscreen<UINT16_MAX) displayModule->vscreen=vscreen;
    }

    /**
     * Перерисовать виджет при ближайшей возможности
     */
    void SetUpdate() {
        if (_displayUpdate) displayModule->SetDisplayUpdate();
        else _needUpdate=true;
    }

};


#endif //MWOS3_ESP32S3_MWOSWIDGET_H
