#ifndef MWOS3_ESP32S3_MWOSWidgetProgressTimeout_H
#define MWOS3_ESP32S3_MWOSWidgetProgressTimeout_H
/**
 * Виджет полосы прогресса таймаута
 * от 0% до 100%
 *
 * Может перересовываться без перерисовки всего дисплея
 */
#include "MWOSWidgetProgressbar.h"
#include "core/adlib/MWTimeout.h"


class MWOSWidgetProgressTimeout: public MWOSWidgetProgressbar {
public:

#pragma pack(push,1)
    uint32_t _timeoutMSec=1;
    MWTimeoutFrom timeout;
    MWTimeout timeoutRefresh;
#pragma pack(pop)

    MWOS_PARAM(11, timeoutMSec, mwos_param_uint32, mwos_param_control, MWOS_STORAGE_NO, 1);

    MWOSWidgetProgressTimeout(MWOSDisplay * _displayModule) : MWOSWidgetProgressbar(_displayModule) {
        AddParam(&p_timeoutMSec);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 11: return _timeoutMSec;
        }
        return MWOSWidgetProgressbar::getValue(param, arrayIndex);
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id==11) StartTimeout(receiverDat->GetValueUInt32());
        MWOSWidgetProgressbar::onReceiveValue(receiverDat);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSWidgetProgressbar::onEvent(modeEvent);
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (IsVisible() && timeout.isStarted() && timeoutRefresh.isTimeout() && _timeoutMSec>0) {
                float percent=((float) timeout.msFromStart())*((float) 100)/((float)_timeoutMSec);
                SetPercent(percent);
                if (percent>=100) timeout.stop(); // прекратим обновление прогресса после 100%
                timeoutRefresh.startMS(100);
            }
        }
    }

    /**
     * Начать показ отсчета времени
     * @param timeoutMSec   таймаут в микросекундах (сек/1000)
     */
    void StartTimeout(uint32_t timeoutMSec) {
        _timeoutMSec=timeoutMSec;
        _visible=true;
        SetParamChanged(&p_timeoutMSec);
        Start();
        timeout.start();
        timeoutRefresh.startMS(100);
    }

};


#endif //MWOS3_ESP32S3_MWOSWidgetProgressTimeout_H
