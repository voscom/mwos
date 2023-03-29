#ifndef MWOS3_MWOSSENSORANALOG_H
#define MWOS3_MWOSSENSORANALOG_H

/***
 * Базовый модуль для аналоговых датчиков
 * Только для наследования из других модулей датчиков!
 *
 * Необходимо в потомках определить параметр портов:
 * MWOS_PARAM(1,sensor_pin, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom,sensorsCount);
 */
#include "MWOSSensorBase.h"

template<uint16_t sensorsCount>
class MWOSSensorAnalog : public MWOSSensorBase<sensorsCount> {
public:
#pragma pack(push,1)
    int32_t _value[sensorsCount];
    int32_t _fromValue;
    int32_t _toValue;
    uint8_t _sensor_digital:1;
    // 0-отправлять только при изменении бинарного значения
    // 1-отправлять при изменении аналогового значения (с учетом фильтров)
    // 2-всегда отправлять показания прошедшие через фильтр (даже когда они не изменялись)
    uint8_t _analog_send:2;
    // сохранять аналоговые показания в журнал (с учетом analog_send и фильтров)
    uint8_t _analog_to_log:1;
    uint8_t _reserve:4;
    uint16_t _analog_filter_min;
    uint16_t _analog_filter_max;
#pragma pack(pop)

    //************************ описание параметров ***********************/

    // последние аналоговые показания
    MWOS_PARAM(9, value, mwos_param_int32, mwos_param_sensor, mwos_param_storage_no, sensorsCount);
    // от какого значения - считать value_bin как бинарная единица
    MWOS_PARAM(10, fromValue, mwos_param_int32, mwos_param_option, mwos_param_storage_eeprom, 1);
    // до какого значения - считать value_bin как бинарная единица
    MWOS_PARAM(11, toValue, mwos_param_int32, mwos_param_option, mwos_param_storage_eeprom, 1);
    // 0-отправлять только при изменении бинарного значения
    // 1-отправлять при изменении аналогового значения (с учетом фильтров)
    // 2-всегда отправлять показания прошедшие через фильтр (даже когда они не изменялись)
    MWOS_PARAM(12, analogSend, mwos_param_bits2, mwos_param_option, mwos_param_storage_eeprom, 1);
    // сохранять аналоговые показания в журнал (с учетом analog_send и фильтров)
    MWOS_PARAM(13, analogToLog, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // аналоговые показания, только если 0. Если 1 - бинарные (при =1 работает полностью аналогично датчику MWOSSensorBin)
    MWOS_PARAM(14, sensorDigital, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    //  игнорирует аналоговые изменения датчика меньше этого значения
    MWOS_PARAM(15, analogFilterMin, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    //  если > 0 - игнорирует аналоговые изменения датчика больше этого значения как сбойные (два таких значения подряд будут пропущены)
    MWOS_PARAM(16, analogFilterMax, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // код ошибки для последней операции чтения показаний датчика
    MWOS_PARAM(17, error, mwos_param_int8, mwos_param_sensor, mwos_param_storage_no, 1);

    MWOSSensorAnalog() : MWOSSensorBase<sensorsCount>() {
        MWOSModuleBase::AddParam(&p_value);
        MWOSModuleBase::AddParam(&p_fromValue);
        MWOSModuleBase::AddParam(&p_toValue);
        MWOSModuleBase::AddParam(&p_analogSend);
        MWOSModuleBase::AddParam(&p_analogToLog);
        MWOSModuleBase::AddParam(&p_sensorDigital);
        MWOSModuleBase::AddParam(&p_analogFilterMin);
        MWOSModuleBase::AddParam(&p_analogFilterMax);
        MWOSModuleBase::AddParam(&p_error);
    }

    virtual void onInit() {
        _fromValue=MWOSModule::loadValue(_fromValue, &p_fromValue,0);
        _toValue=MWOSModule::loadValue(_toValue, &p_toValue,0);
        _analog_send=MWOSModule::loadValue(_analog_send, &p_analogSend,0);
        _analog_to_log=MWOSModule::loadValue(_analog_to_log, &p_analogToLog,0);
        _analog_filter_min=MWOSModule::loadValue(_analog_filter_min, &p_analogFilterMin,0);
        _analog_filter_max=MWOSModule::loadValue(_analog_filter_max, &p_analogFilterMax,0);
        _sensor_digital=MWOSModule::loadValue(_sensor_digital, &p_sensorDigital,0);
        MWOSSensorBase<sensorsCount>::onInit();
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex= 0) {
        if (param->id==9) return _value[arrayIndex];
        if (param->id==15) return getErrorCode();
        return MWOSSensorBase<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

   virtual bool readBoolValue(uint16_t arrayIndex) {
       if (_sensor_digital) return readDigitalValue(arrayIndex); // это бинарный датчик - просто считам бинарное значение
       int32_t newValue = readAnalogValue(arrayIndex);
       if (getErrorCode()!=0) { // была ошибка чтения
           return MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].bin_value;
       }
       bool analogChanged=MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed; // второй бит значения - было аналоговое изменение
       if (newValue != _value[arrayIndex]) {
           MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed=1;
           analogChanged=true;
       }
       if (analogChanged) {
           int32_t changedValue=newValue - _value[arrayIndex];
           changedValue=abs(changedValue);
           if (_analog_filter_min>0 && _analog_filter_min>changedValue) {// слишком слабо изменилось значение датчика - игнорируем
               MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed=0; // analogChanged=false; // сбросим аналоговое изменение
               MW_LOG_MODULE(this); MW_LOG(F("Skip min>change=value: ")); MW_LOG(_analog_filter_min); MW_LOG('>'); MW_LOG(changedValue); MW_LOG('='); MW_LOG_LN(newValue);
               return MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].bin_value;
           }
           if (_analog_filter_max>0 && _analog_filter_max<changedValue) { // слишком сильно изменилось значение датчика - дождемся повтора измерения
               _value[arrayIndex]=newValue;
               MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].analog_changed=0; // analogChanged=false; // сбросим аналоговое изменение
               MW_LOG_MODULE(this); MW_LOG(F("Skip max<change=value: ")); MW_LOG(_analog_filter_max); MW_LOG('<'); MW_LOG(changedValue); MW_LOG('='); MW_LOG_LN(newValue);
               return MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].bin_value;
           }
           _value[arrayIndex]=newValue;
       }
       bool newValueBin= (newValue >= _fromValue) && (newValue <= _toValue);
       bool binChanged=(newValueBin != MWOSSensorBase<sensorsCount>::_value_bin[arrayIndex].bin_value);
       if ((_analog_send==0 && binChanged) || (_analog_send==1 && analogChanged) || (_analog_send==2)) {
           if (_analog_to_log) MWOSModule::toLog(newValue, &p_value, arrayIndex);
           else MWOSModule::SetParamChanged(&p_value, arrayIndex, true);
       }
       return newValueBin;
    }

    /**
     * Опросить аналоговый датчик для получения новых показаний
     */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        return 0;
    }

    virtual int32_t readDigitalValue(int16_t arrayIndex) {
        return 0;
    }

    /***
     * Возвращает код ошибки для последней операции чтения показаний датчика
     * @return
     */
    virtual int8_t getErrorCode() {
        return 0;
    }

};


#endif //MWOS3_MWOSSENSORANALOG_H
