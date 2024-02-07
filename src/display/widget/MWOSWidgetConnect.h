#ifndef MWOS3_ESP32S3_MWOSWIDGETCONNECT_H
#define MWOS3_ESP32S3_MWOSWIDGETCONNECT_H
/**
 * Виджет дисплея MWOS
 *
 * Показывает статус подключения к серверу MWOS и сети
 *
 */
#include <Adafruit_GFX.h>
#include "MWOSWidget.h"
#include "core/net/MWOSNetModule.h"

// картинка антенны
static const unsigned char PROGMEM bmpConnect[] =
        { 0b01111111, 0b11111110,
          0b11111111, 0b11111111,
          0b00000000, 0b00000000,
          0b00000000, 0b00000000,
          0b00111111, 0b11111100,
          0b00011111, 0b11111000,
          0b00000000, 0b00000000,
          0b00000000, 0b00000000,
          0b00001111, 0b11110000,
          0b00000111, 0b11100000,
          0b00000000, 0b00000000,
          0b00000000, 0b00000000,
          0b00000011, 0b11000000,
          0b00000011, 0b11000000,
          0b00000011, 0b11000000};
#define bmpConnect_width 16
#define bmpConnect_height 15


class MWOSWidgetConnect: public MWOSWidget {
public:

#pragma pack(push,1)
    uint8_t _connected=255; // тип подключения к сети и серверу (0-нет, 1-подключено к сети, 2-подключено к серверу)
    MWOSNetStep lastConnectedStep;
#pragma pack(pop)

    MWOS_PARAM(10, connect, mwos_param_bits2, mwos_param_readonly, mwos_param_storage_no, 1);

    MWOSWidgetConnect(MWOSDisplay * _displayModule) : MWOSWidget(_displayModule,(char *) F("widgetConnect")) {
        AddParam(&p_connect);
        _color=WHITE;
        _x= displayModule->display->width() - bmpConnect_width - 1; // по умолчанию - покажем в правом верхнем углу
        _y=0;
    }

    virtual void onInit() {
        MWOSWidget::onInit();
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 10: return _connected;
        }
        return MWOSWidget::getValue(param, arrayIndex);
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (((MWOSNetModule *) mwos.netModule)->connectedStep!=lastConnectedStep) {
            lastConnectedStep=((MWOSNetModule *) mwos.netModule)->connectedStep;
            uint8_t nowConnected;
            if (lastConnectedStep==STEP_SERVER_CONNECTED) nowConnected=2;
            else
            if (lastConnectedStep>=STEP_NET_CONNECTING) nowConnected=1;
            else nowConnected=0;
            if (_connected!=nowConnected) {
                _connected=nowConnected;
                SetParamChanged(&p_connect,0, true);
                displayModule->SetDisplayUpdate();
            }
        }
        MWOSWidget::onUpdate();
    }

    /**
     * Показать статус подключения на дисплее
     */
    void print() {
        MW_LOG_MODULE(this); MW_LOG(F("print: ")); MW_LOG_LN(_connected);
        if (_connected>0) displayModule->display->drawBitmap(_x, _y, bmpConnect, bmpConnect_width, bmpConnect_height, _color);
        if (_connected<2) {
            displayModule->display->drawLine(_x - 1, _y + 3, _x + bmpConnect_width, _y + bmpConnect_height, _color);
            displayModule->display->drawLine(_x + bmpConnect_width, _y + 3, _x - 1, _y + bmpConnect_height, _color);
        }
    }



};


#endif //MWOS3_ESP32S3_MWOSWIDGETCONNECT_H
