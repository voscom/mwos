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

#ifndef MWOS3_ESP32S3_MWOSWIDGET_H
#define MWOS3_ESP32S3_MWOSWIDGET_H

#include <Adafruit_GFX.h>
#include "display/MWOSDisplay.h"

class MWOSWidget : public MWOSModule {
public:

#pragma pack(push,1)
    MWOSDisplay * displayModule;
    int16_t _x =0;
    int16_t _y =0;
    uint16_t _sizeX =1;
    uint16_t _sizeY =1;
    int16_t _color =WHITE;
#pragma pack(pop)

    // координата x
    MWOS_PARAM(0, x, mwos_param_int16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // координата y
    MWOS_PARAM(1, y, mwos_param_int16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // размер по X
    MWOS_PARAM(2, sizeX, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // размер по Y
    MWOS_PARAM(3, sizeY, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // цвет текста
    MWOS_PARAM(4, color, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);

    MWOSWidget(MWOSDisplay * _displayModule, char * unit_name) : MWOSModule(unit_name) {
        displayModule=_displayModule;
        AddParam(&p_x);
        AddParam(&p_y);
        AddParam(&p_sizeX);
        AddParam(&p_sizeY);
        AddParam(&p_color);
    }

    virtual void onInit() {
        _x=MWOSWidget::loadValue(_x, &p_x, 0);
        _y=MWOSWidget::loadValue(_y, &p_y, 0);
        _sizeX=MWOSWidget::loadValue(_sizeX, &p_sizeX, 0);
        _sizeY=MWOSWidget::loadValue(_sizeY, &p_sizeY, 0);
        _color=MWOSWidget::loadValue(_color, &p_color, 0);
        MWOSModule::onInit();
    }

    /***
      * Вызывается каждый тик операционной системы
      */
    virtual void onUpdate() {
        if (displayModule->IsNeedUpdated()) print();
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {

    }

    /***
     * Задать параметры по умолчанию для этого виджета (эти параметры могут быть изменены удаленно)
     * @param x     Координата X на дисплее
     * @param y     Координата Y на дисплее
     * @param sizeX Размер по X
     * @param sizeY Размер по Y
     * @param color Цвет
     */
    void setDefault(int16_t x, int16_t y, uint8_t sizeX, uint8_t sizeY, uint16_t color) {
        _x=x;
        _y=y;
        _sizeX=sizeX;
        _sizeY=sizeY;
        _color=color;
    }



};


#endif //MWOS3_ESP32S3_MWOSWIDGET_H
