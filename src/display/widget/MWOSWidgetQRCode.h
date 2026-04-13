#ifndef MWOS3_ESP32S3_MWOSWIDGETQRCODE_H
#define MWOS3_ESP32S3_MWOSWIDGETQRCODE_H
#include "MWOSWidget.h"
#include "qrcode.h"

/**
 * Виджет дисплея MWOS.
 * Показывает QR-код для заданной строки.
 * Использует библиотеку:
 * https://github.com/ricmoo/QRCode.git
 *
 * @tparam stringLength    Максимальный размер строки
 */
template<MWOS_PARAM_INDEX_UINT stringLength>
class MWOSWidgetQRCode: public MWOSWidget {
public:

    String _text="";

    // тест для показа на дисплее (до stringLength символов)
    MWOS_PARAM(10, text, mwos_param_string, mwos_param_option, MWOS_STORAGE_NVS, stringLength);

    MWOSWidgetQRCode(MWOSDisplay * _displayModule) : MWOSWidget(_displayModule,(char *) F("widgetQRCode")) {
        AddParam(&p_text);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 10: {
                if (arrayIndex>=_text.length()) return 0; // дополняем текст справа нулями до заданной длины
                return _text[arrayIndex];
            } break;
        }
        return MWOSWidget::getValue(param, arrayIndex);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы

        } else if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _text = MWOSWidget::loadValueString(_text, &p_text);
        }
        MWOSWidget::onEvent(modeEvent);
    }
    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id==10) {
            setText(receiverDat->GetValueString());
            MWOSWidget::saveValueStr(_text,&p_text);
            MWOSWidget::SetParamChanged(&p_text);
        } else
            MWOSWidget::onReceiveValue(receiverDat);
    }

    /**
     * Задать параметры текста
     * @param text      Сам текст
     */
    void setText(String text) {
        _text=text;
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        if (!IsVisible()) return;
        if (_displayUpdate==0) displayModule->fillRect(_x,_y,_width,_height,displayModule->_backColor); // сотрем область виджета
        QRCode qrcode;
        uint8_t v=3;
        if (stringLength>279) v=9;
        else if (stringLength>224) v=8;
        else if (stringLength>195) v=7;
        else if (stringLength>154) v=6;
        else if (stringLength>114) v=5;
        else if (stringLength>77) v=4;
        //else if (stringLength>47) v=3;
        //else if (stringLength>25) v=2;
        uint8_t qrcodeData[qrcode_getBufferSize(v)];
        qrcode_initText(&qrcode, qrcodeData, v, 0, _text.c_str()); // 2=25x25, 0=MEDIUM (до 47 символов)
        int16_t xScale=_width/qrcode.size;
        int16_t yScale=_height/qrcode.size;
        int16_t scale;
        if (yScale>xScale) scale=xScale;
        else scale=yScale;
        if (scale<1) scale=1;
        int16_t w=qrcode.size*scale;
        int16_t h=qrcode.size*scale;
        for (int16_t y = 0; y < qrcode.size; y++) {
            for (int16_t x = 0; x < qrcode.size; x++) {
                int16_t x1=alignX(_x+x*scale,_width,w);
                int16_t y1=alignY(_y+y*scale,_height,h);
                if (qrcode_getModule(&qrcode, x, y)) {
                    displayModule->fillRect(x1,y1,scale,scale,_color);
                } else {
                    //displayModule->fillRect(x1,y1,scale,scale,displayModule->_backColor);
                }
            }
        }
        MWOSWidget::print();
    }

};


#endif //MWOS3_ESP32S3_MWOSWIDGETQRCODE_H
