#ifndef MWOS3_ESP32S3_MWOSWidgetProgressbar_H
#define MWOS3_ESP32S3_MWOSWidgetProgressbar_H
/**
 * Виджет полосы прогресса
 * от 0% до 100%
 *
 * Может перересовываться без перерисовки всего дисплея
 */
#include "MWOSWidget.h"


class MWOSWidgetProgressbar: public MWOSWidget {
public:

#pragma pack(push,1)
    float _percent=0;
    int16_t lastWidth=-1;
#pragma pack(pop)

    MWOS_PARAM(10, percent, mwos_param_float32, mwos_param_control, MWOS_STORAGE_NO, 1);

    MWOSWidgetProgressbar(MWOSDisplay * _displayModule) : MWOSWidget(_displayModule,(char *) F("widgetProgressbar")) {
        AddParam(&p_percent);
        _color=MWOS_COLOR_WHITE;
        _x= 0;
        _y= displayModule->height()-16; // по умолчанию - покажем внизу
        _width =displayModule->width();
        _height =16;
        _visible= true;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 10: return param->resultFloat(_percent);
        }
        return MWOSWidget::getValue(param, arrayIndex);
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id==10) SetPercent(receiverDat->GetValueFloat());
        MWOSWidget::onReceiveValue(receiverDat);
    }

    /**
     * Начать отрисовку прогресса
     */
    void Start() {
        SetPercent(0);
        lastWidth=0;
    }

    /**
     * Задать новый процент
     * @param newPercent
     */
    void SetPercent(float newPercent) {
        if (_percent!=newPercent) SetParamChanged(&p_percent);
        _percent=newPercent;
        if (_percent<0) _percent=0;
        if (_percent>100) _percent=100;
        SetUpdate();
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        if (_visible==0) return;
        // рамку рисуем только после перерисовки всего дисплея
        if (_percent<0) _percent=0;
        if (_percent>100) _percent=100;
        // MW_LOG_MODULE(this); MW_LOG(_percent); MW_LOG('*'); MW_LOG_LN(_width);
        float width=_percent*(((float)(_width-2))/((float)100));
        int16_t widthInt=round(width);
        if (displayModule->IsNeedUpdated() || (lastWidth<0)) { // если обновляется весь дисплей или первый кадр
            // MW_LOG_MODULE(this); MW_LOG(F("printAll ")); MW_LOG_LN(widthInt);
            if (_displayUpdate==0) displayModule->fillRect(_x,_y,_width,_height,displayModule->_backColor); // сотрем область виджета
            displayModule->drawRect(_x,_y,_width, _height,_color);
            if (widthInt>0) displayModule->fillRect(_x+1,_y+1,widthInt-1, _height-2,_color); // перерисуем полностью
            lastWidth=widthInt;
        } else
        if (widthInt!=lastWidth) { // заливка только при изменении или перерисовке дисплея
            // MW_LOG_MODULE(this); MW_LOG(F("printPart ")); MW_LOG_LN(widthInt);
            if (widthInt>lastWidth) displayModule->fillRect(_x+1+lastWidth,_y+1,widthInt-lastWidth, _height-2,_color); // дорисуем кусочек
            if (widthInt<lastWidth) displayModule->fillRect(_x+1+widthInt,_y+1,lastWidth-widthInt, _height-2,displayModule->_backColor); // сотрем кусочек
            lastWidth=widthInt;
        }
        MWOSWidget::print();
    }



};


#endif //MWOS3_ESP32S3_MWOSWidgetProgressbar_H
