#ifndef MWOS3_ESP32S3_MWOSWIDGETBATTERY_H
#define MWOS3_ESP32S3_MWOSWIDGETBATTERY_H
#include "MWOSWidgetProgressbar.h"
#include "core/net/MWOSNetModule.h"
#include "core/MWOSLinkToValue.h"

/**
 * Виджет заряда батареи
 * Обычное напряжение показывает от минимума до максимума датчика
 * зарядку показывает от chargeMin до chargeMax
 * chargeMin должно быть выше максимума датчика
 *
 * Изменяет цвет:
 * зеленый - батарея больше 80%
 * желтый  - батарея меньше 80%
 * красный - батарея меньше 40%
 * синий   - идет зарядка
 * _color (задано в параметре) - батарея заряжена и на зарядке
 *
 */
class MWOSWidgetBattery: public MWOSWidgetProgressbar {
public:

    int32_t _lastValue=0;
    bool chargingMode; // сейчас режим зарядки

    // ссылки на 2 параметра: 0=значение зарядки, 1=бинарный признак зарядки
    MWOS_PARAMS_LINK_TO_MODULE_PARAM_VALUE(11,12,13,2);


    MWOSWidgetBattery(MWOSDisplay * _displayModule) : MWOSWidgetProgressbar(_displayModule) {
        setName((char *) F("widgetBattery"));
        AddParamsLinkToValue();
        _color=MWOS_COLOR_GRAY;
        _x= displayModule->width() - 40; // по умолчанию - покажем в правом верхнем углу - 20
        _y=4;
        vscreen=MWOS_SCREEN_ALL; // покажем на всех экранах
        _visible= true;
        _width=16;
        _height=8;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (IsIdParamForLink(param->id)) return IdParamForLink(param->id,arrayIndex);
        return MWOSWidgetProgressbar::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSWidgetProgressbar::onEvent(modeEvent);
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (GetValueParamLinkToValue(0)) { // получим значение датчика в _linkParamValue[0]
                int32_t nowValue=_linkParamValue[0];
                if (_lastValue!=nowValue) { // изменилось значение напряжения
                    // просто напряжение на батарее
                    int32_t _batMin=_linkToModule[0]->getValueByParamId(10, _linkToValueIndex[0]);
                    int32_t _batMax=_linkToModule[0]->getValueByParamId(11, _linkToValueIndex[0]);
                    SetPercent(getBatteryPercent(nowValue,_batMin,_batMax));
                    _lastValue=nowValue;
                }
            }
            GetValueParamLinkToValue(1);
            chargingMode=(_linkParamValue[1]>0);  // это идет заряд батареи
        } else if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) {
            LoadParamsAllLinkToValue();
        }
    }

    float getBatteryPercent(int32_t nowValue,int32_t minValue, int32_t maxValue) {
        if (nowValue<=minValue || minValue>=maxValue) return 0;
        if (nowValue>=maxValue) return 100;
        float maxValueF = maxValue-minValue;
        float nowValueF = nowValue-minValue;
        return nowValueF / maxValueF * 100;
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        if (_visible==0) return;
        if (MWOS_COLOR_WHITE!=1) { // RGB дисплей - поменяем цвет
            /*
            if (chargingMode) {
                if (_percent>80) _color=0b0111111111011110;
                else _color=0b0111101111111111;
            } else {
                if (_percent>80) _color=0b0111111111011110;
                else
                if (_percent>40) _color=0b1111111111011110;
                else _color=0b1111101111011110;
            }
             */
        }
        int16_t h0=_height/4;
        int16_t w0=h0/3;
        if (w0<2) w0=2;
        displayModule->fillRect( _x+_width, _y+_height/2-h0/2, w0, h0, _color); // нарисуем пипку
        if (chargingMode!=lastChargingMode) {
            lastWidth=-1;
            lastChargingMode=chargingMode;
        }
        MWOSWidgetProgressbar::print();
        if (chargingMode) {
            displayModule->drawLine( _x+_width/2,_y+_height/2-3, _x+_width/2,_y+_height/2+3, MWOS_COLOR_RED);
            displayModule->drawLine( _x+_width/2-3,_y+_height/2, _x+_width/2+3,_y+_height/2, MWOS_COLOR_RED);
        }
    }

private:
    bool lastChargingMode;

};


#endif //MWOS3_ESP32S3_MWOSWIDGETBATTERY_H
