#ifndef MWOS3_ESP32S3_MWOSWIDGETCONNECT_H
#define MWOS3_ESP32S3_MWOSWIDGETCONNECT_H
/**
 * Виджет дисплея MWOS
 *
 * Показывает статус подключения к серверу MWOS и сети
 *
 */
#include "MWOSWidget.h"
#include "core/net/MWOSNetModule.h"

#define bmpConnect_width 16
#define bmpConnect_height 8

enum ConnectedMode : uint8_t {
    CM_DISCONNECTED=0,
    CM_CONNECTEDNET=1,
    CM_CONNECTEDSERVER=2,
    CM_SESSION=3,
    CM_NETSETUP=4,
    CM_NONE=255,
};

class MWOSWidgetConnect: public MWOSWidget {
public:

#pragma pack(push,1)
    ConnectedMode _connected=CM_NONE; // тип подключения к сети и серверу (0-нет, 1-подключено к сети, 2-подключено к серверу)
    MWOSNetStep lastConnectedStep;
    mwos_color _colors[5]={MWOS_COLOR_GRAY,MWOS_COLOR_RED,MWOS_COLOR_YELLOW,MWOS_COLOR_GREEN,MWOS_COLOR_BLUE};
#pragma pack(pop)

    MWOS_PARAM(10, connect, mwos_param_bits2, mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // цвета подключения (по ConnectedMode)
    MWOS_PARAM(11, colors, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 5);

    MWOSWidgetConnect(MWOSDisplay * _displayModule) : MWOSWidget(_displayModule,(char *) F("widgetConnect")) {
        AddParam(&p_connect);
        AddParam(&p_colors);
        _color=MWOS_COLOR_WHITE;
        _x= displayModule->width() - bmpConnect_width - 1; // по умолчанию - покажем в правом верхнем углу
        _y=0;
        vscreen=MWOS_SCREEN_ALL; // покажем на всех экранах
        _visible= true;
        _width=bmpConnect_width;
        _height=bmpConnect_height;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 10: return _connected;
            case 11: return _colors[arrayIndex];
        }
        return MWOSWidget::getValue(param, arrayIndex);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (!IsVisible()) return;
            if (!MWOSNetModule::getNetModule()->IsActive()) { // сеть не активна
                _connected = CM_NONE;
                lastConnectedStep = MWOSNetModule::getNetModule()->connectedStep;
                return;
            }
            if (MWOSNetModule::getNetModule()->connectedStep != lastConnectedStep) {
                ConnectedMode nowConnected=CM_DISCONNECTED;
                if (MWOSNetModule::getNetModule()->IsSetupMode) nowConnected = CM_NETSETUP;
                else
                if (MWOSNetModule::getNetModule()->IsConnected) {
                  if (MWOSNetModule::getNetModule()->_startSession) nowConnected = CM_SESSION;
                  else nowConnected = CM_CONNECTEDSERVER;
                } else
                if (MWOSNetModule::getNetModule()->isConnectedNet()) nowConnected = CM_CONNECTEDNET;
                if (_connected != nowConnected) {
                    _connected = nowConnected;
                    SetParamChanged(&p_connect, 0, true);
                    SetUpdate();
                }
            }
        } else {
            if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) {
                for (uint16_t i = 0; i < 5; i++) {
                    _colors[i] = loadValue(_colors[i], &p_colors, i);
                }
            }
            if (modeEvent==EVENT_INIT) {
                _x= displayModule->width() - bmpConnect_width - 1;
            }
        }
        MWOSWidget::onEvent(modeEvent);
    }

    /**
     * Показать статус подключения на дисплее
     */
    void print() {
        if (_visible==0) return;
        MW_LOG_MODULE(this); MW_LOG(F("print: ")); MW_LOG_LN(_connected);
        if (_displayUpdate==0) displayModule->fillRect(_x,_y,_width,_height,displayModule->_backColor); // сотрем область виджета
        if (_connected<5) _color=_colors[_connected];

        int16_t hW=(_width-3)/4;
        int16_t hH=_height/4;
        for (int16_t i=0; i<4; i++) {
            displayModule->fillRect(_x+i*(hW+1), _y+(3-i)*hH, hW, hH*(i+1),_color);
        }
        if (_connected<CM_CONNECTEDSERVER) {
            displayModule->drawLine(_x , _y , _x + _width-2, _y +_height-1, _color);
            displayModule->drawLine(_x+1 , _y , _x + _width-1, _y +_height-1, _color);
        }
        MWOSWidget::print();
    }



};


#endif //MWOS3_ESP32S3_MWOSWIDGETCONNECT_H
