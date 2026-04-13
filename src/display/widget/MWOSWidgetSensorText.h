#ifndef MWOS3_ESP32S3_MWOSWIDGETSENSORTEXT_H
#define MWOS3_ESP32S3_MWOSWIDGETSENSORTEXT_H
//
// Виджет дисплея MWOS
//
#include "MWOSWidgetTextValue.h"
#include "../../core/MWOSLinkToValue.h"

#ifndef MWOSWidgetSensor_UPDATE_TIMEOUT_DSEC
#define MWOSWidgetSensor_UPDATE_TIMEOUT_DSEC 3 // [сек/10] таймаут обновления значений
#endif
/**
 * Виджет показывает текст на экране
 * Доп.шрифты тут:
 * https://github.com/immortalserg/AdafruitGFXRusFonts
 *
 * @tparam stringLength    Максимальный размер строки
 */
template<MWOS_PARAM_INDEX_UINT stringLength>
class MWOSWidgetSensorText: public MWOSWidgetTextValue<stringLength> {
public:

#pragma pack(push,1)
    uint8_t _digits=0;
    MWTimeout timeout;
#pragma pack(pop)

    // количество знаков после запятой
    MWOS_PARAM(21, digits, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    // добавим параметры и методы линковки на значения другого параметра в другом модуле
    MWOS_PARAMS_LINK_TO_MODULE_PARAM_VALUE(22,23,24,1);


    MWOSWidgetSensorText(MWOSDisplay * _displayModule) : MWOSWidgetTextValue<stringLength>(_displayModule) {
        MWOSWidget::name=(char *) F("widgetSensorText");
        MWOSModule::AddParam(&p_digits);
        AddParamsLinkToValue();
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSWidgetTextValue<stringLength>::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _digits = MWOSWidgetTextValue<stringLength>::loadValue(_digits, &p_digits, 0);
            LoadParamsAllLinkToValue();
            timeout.start(MWOSWidgetSensor_UPDATE_TIMEOUT_DSEC);
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (!MWOSWidgetTextValue<stringLength>::IsVisible()) return;
            if (timeout.isTimeout()) {
                if (GetValueParamLinkToValue(0)) {
                    if (_linkToModuleParam[0]->IsFloat()) {
                        MWOSWidgetTextValue<stringLength>::setValue(String(_linkToModuleParam[0]->valueToFloat(_linkParamValue[0]), (unsigned int) _digits));
                    } else
                    if (_digits>0) {
                        if (_digits>15) _digits=15; // максимум - 15 знаков после запятой
                        float floatSensorValue=_linkParamValue[0];
                        for (uint8_t i = 0; i < _digits; ++i) floatSensorValue/=10.0;
                        MWOSWidgetTextValue<stringLength>::setValue(String(floatSensorValue,(unsigned int) _digits));
                    } else {
                        MWOSWidgetTextValue<stringLength>::setValue(String(_linkParamValue[0]));
                    }
                }
                timeout.start(MWOSWidgetSensor_UPDATE_TIMEOUT_DSEC);
            }
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (IsIdParamForLink(param->id)) return IdParamForLink(param->id,arrayIndex);
        else
        switch (param->id) {
            case 21: return _digits;
        }
        return MWOSWidgetTextValue<stringLength>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    void setDefaultParam(MWOSModuleBase * module, MWOS_PARAM_UINT paramId, MWOS_PARAM_INDEX_UINT index, uint8_t digits) {
        setDefaultParamLinkToValue(0,module,paramId,index);
        _digits=digits;
    }

};


#endif //MWOS3_ESP32S3_MWOSWIDGET_H
