#ifndef MWOS3_ESP32S3_MWOSWIDGETSENSORTEXT_H
#define MWOS3_ESP32S3_MWOSWIDGETSENSORTEXT_H
//
// Виджет дисплея MWOS
//
#include <Adafruit_GFX.h>
#include "MWOSWidgetText.h"

class MWOSWidgetSensorText: public MWOSWidgetText {
public:

#pragma pack(push,1)
    MWOSModuleBase * _sensor;
    MWOSParam * sensorParam;
    MWOS_PARAM_UINT _sensor_id;
    MWOS_PARAM_INDEX_UINT _sensorIndex;
    MWTimeout timeout;
    int64_t lastSensorValue=INT64_MIN;
#pragma pack(pop)

    // id модуля аналогового датчика
    MWOS_PARAM(20, sensorModuleId, MWOS_PARAM_UINT_PTYPE, mwos_param_option, mwos_param_storage_eeprom, 1);
    // индекс параметра
    MWOS_PARAM(21, sensorIndex, MWOS_PARAM_INDEX_UINT_PTYPE, mwos_param_option, mwos_param_storage_eeprom, 1);

    MWOSWidgetSensorText(MWOSDisplay * _displayModule) : MWOSWidgetText(_displayModule) {
        name=(char *) F("widgetSensorText");
        AddParam(&p_sensorModuleId);
        AddParam(&p_sensorIndex);
    }

    virtual void onInit() {
        _sensor=NULL;
        _sensorIndex=MWOSWidgetText::loadValue(_sensorIndex, &p_sensorIndex, 0);
        if (_sensorIndex<0) {
            MW_LOG_MODULE(this); MW_LOG(F("error sensor index: ")); MW_LOG_LN(_sensorIndex);
            return;
        }
        _sensor_id=MWOSWidgetText::loadValue(_sensor_id, &p_sensorModuleId, 0);
        if (_sensor_id<2) {
            MW_LOG_MODULE(this); MW_LOG(F("error sensor module id: ")); MW_LOG_LN(_sensor_id);
            return;
        }
        _sensor=mwos.getModule(_sensor_id);
        if (_sensor==NULL) {
            MW_LOG_MODULE(this); MW_LOG(F("error sensor not found: ")); MW_LOG_LN(_sensor_id);
            return;
        }
        sensorParam=_sensor->getParam(MWOS_SENSOR_ANALOG_PARAM_ID);
        if (sensorParam==NULL || sensorParam->group!=mwos_param_readonly) { // любой другой тип, кроме ридонли
            MW_LOG_MODULE(this); MW_LOG_LN(F("error param type!"));
            sensorParam=NULL;
            _sensor=NULL; // удалим не датчики
            return;
        }
        MW_LOG_MODULE(this); MW_LOG_LN(F("onInited!"));
        timeout.start(2);
        MWOSWidgetText::onInit();
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (timeout.isTimeout()) {
            if (_sensor!=NULL && _sensorIndex>=0) {
                int64_t nowSensorValue=_sensor->getValue(sensorParam,_sensorIndex);
                if (lastSensorValue!=nowSensorValue) {
                    print(String(nowSensorValue));
                    lastSensorValue=nowSensorValue;
                }
            }
            timeout.start(2);
        }
        MWOSWidgetText::onUpdate();
    }

    void setDefaultSensor(MWOSModuleBase * module, MWOS_PARAM_INDEX_UINT index) {
        _sensor_id=module->id;
        _sensorIndex=index;
    }

};


#endif //MWOS3_ESP32S3_MWOSWIDGET_H
