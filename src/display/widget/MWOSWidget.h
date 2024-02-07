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
    int8_t _visible =0;
    int16_t _x =0;
    int16_t _y =0;
    uint16_t _sizeX =1;
    uint16_t _sizeY =1;
    int16_t _color =WHITE;
#pragma pack(pop)

    // видимость виджета
    MWOS_PARAM(0, visible, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // координата x
    MWOS_PARAM(1, x, mwos_param_int16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // координата y
    MWOS_PARAM(2, y, mwos_param_int16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // размер по X
    MWOS_PARAM(3, sizeX, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // размер по Y
    MWOS_PARAM(4, sizeY, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // цвет текста
    MWOS_PARAM(5, color, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);

    MWOSWidget(MWOSDisplay * _displayModule, char * unit_name) : MWOSModule(unit_name) {
        displayModule=_displayModule;
        AddParam(&p_visible);
        AddParam(&p_x);
        AddParam(&p_y);
        AddParam(&p_sizeX);
        AddParam(&p_sizeY);
        AddParam(&p_color);
    }

    virtual void onInit() {
        _visible=MWOSWidget::loadValue(_visible, &p_visible, 0);
        _x=MWOSWidget::loadValue(_x, &p_x, 0);
        _y=MWOSWidget::loadValue(_y, &p_y, 0);
        _sizeX=MWOSWidget::loadValue(_sizeX, &p_sizeX, 0);
        _sizeY=MWOSWidget::loadValue(_sizeY, &p_sizeY, 0);
        _color=MWOSWidget::loadValue(_color, &p_color, 0);
        MW_LOG_MODULE(this); MW_LOG(F("onInit: ")); MW_LOG(_x); MW_LOG(';'); MW_LOG(_y); MW_LOG('s'); MW_LOG(_sizeX); MW_LOG(';'); MW_LOG(_sizeY); MW_LOG('c'); MW_LOG_LN(_color);
        MWOSModule::onInit();
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 0: return _visible;
            case 1: return _x;
            case 2: return _y;
            case 3: return _sizeX;
            case 4: return _sizeY;
            case 5: return _color;
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
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
    void setDefault(bool visible, int16_t x, int16_t y, uint8_t sizeX, uint8_t sizeY, uint16_t color) {
        _visible=visible;
        _x=x;
        _y=y;
        _sizeX=sizeX;
        _sizeY=sizeY;
        _color=color;
    }



};


#endif //MWOS3_ESP32S3_MWOSWIDGET_H
