#ifndef MWOS3_ESP32S3_MWOSWidgetTextValue_H
#define MWOS3_ESP32S3_MWOSWidgetTextValue_H
/*
 * Виджет текстового значения
 * добавляет текстовое значение к тексту
 * если в тексте есть макрос %0 - вставляет значение вместо макроса
 */
#include "MWOSWidgetText.h"

/**
 * Виджет показывает текст на экране
 * добавляет текстовое значение к тексту
 * если в тексте есть макрос %0 - вставляет значение вместо макроса
 * сам текст хранится в EEPROM и может быть изменен удаленно, а значение генерируется
 *
 * @tparam stringLength    Максимальный размер строки и значения
 */
template<MWOS_PARAM_INDEX_UINT stringLength>
class MWOSWidgetTextValue: public MWOSWidgetText<stringLength> {
public:

    String _value="";

    // текстовое значение, добавляемое к тексту из параметра p_text
    MWOS_PARAM(20, value, mwos_param_string, mwos_param_control, MWOS_STORAGE_NO, MWOS_SEND_BUFFER_SIZE-512);

    MWOSWidgetTextValue(MWOSDisplay * _displayModule) : MWOSWidgetText<stringLength>(_displayModule) {
        MWOSWidgetText<stringLength>::name=(char *) F("widgetTextValue");
        MWOSWidgetText<stringLength>::AddParam(&p_value);
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id==20) {
            setValue(receiverDat->GetValueString());
        } else
            MWOSWidgetText<stringLength>::onReceiveValue(receiverDat);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 20: {
                if (arrayIndex>=_value.length()) return 0;
                return _value[arrayIndex];
            }
        }
        return MWOSWidgetText<stringLength>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual void print() {
        if (!MWOSWidgetText<stringLength>::IsVisible()) return;
        String nowStr=MWOSWidgetText<stringLength>::_text;
        if (nowStr=="") MWOSWidgetText<stringLength>::_text=_value;
        else {
            int16_t p=nowStr.indexOf("%0");
            if (p>=0) {
                MWOSWidgetText<stringLength>::_text=nowStr.substring(0,p)+_value;
                MWOSWidgetText<stringLength>::_text+=nowStr.substring(p+2);
            } else {
                MWOSWidgetText<stringLength>::_text=nowStr+_value;
            }
        }
        MWOSWidgetText<stringLength>::print();
        MWOSWidgetText<stringLength>::_text=nowStr;
    }

    void setValue(const String &value) {
        _value=value;
        MWOSWidgetText<stringLength>::SetParamChanged(&p_value,0,true);
        if (MWOSWidgetText<stringLength>::IsVisible()) MWOSWidgetText<stringLength>::SetUpdate();
        MW_LOG_MODULE(this); MW_LOG(F("setValue: ")); MW_LOG_LN(_value);
    }

    void setDefaultValue(const String &value) {
        _value=value;
    }

};


#endif //MWOS3_ESP32S3_MWOSWIDGET_H
