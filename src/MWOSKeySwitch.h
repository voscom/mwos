#ifndef MWOS3_MWOSKEYSWITCH_H
#define MWOS3_MWOSKEYSWITCH_H
/***
 * Переключатель (ключ, переключаемый датчиком)
 * Управляется заданным датчиком. При изменении бинарного значения датчика - переключает ключ.
 */

#include "MWOSKeyExt.h"
#include "core/MWOSSensorBase.h"

#ifndef MWOSKeyForSwitch // можно переопределить наследников для переключателей (MWOSKey или его потомки)
#if (MWOS_MINIMUM_RAM==1)
#define MWOSKeyForSwitch MWOSKey
#else
#define MWOSKeyForSwitch MWOSKeyExt
#endif
#endif

template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKeySwitch : public MWOSKeyForSwitch<keysCount> {
public:
#pragma pack(push,1)
    MWOSModuleBase * _sensor;
    MWOS_PARAM_INDEX_UINT _pIndex[keysCount];
    MWBitsMaskStat<keysCount> _lastSensorsValue;
    MWTimeout initTimeout;
#pragma pack(pop)


    // ************ описание параметров *****************
    // датчик для управления этим переключателем
    MWOS_PARAM(21, sensorModuleId, MWOS_PARAM_UINT_PTYPE, mwos_param_module_id+mwos_param_option, mwos_param_storage_eeprom, 1);
    // индексы параметра 0 этого датчика (если там много однотипных датчиков, то каждый из этих датчиков может управлять своим ключом в этом модуле)
    MWOS_PARAM(22, sensorIndex, MWOS_PARAM_INDEX_UINT_PTYPE, mwos_param_option, mwos_param_storage_eeprom, keysCount);

    /***
     * Создать ключи
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSKeySwitch(const uint8_t * pins) : MWOSKeyForSwitch<keysCount>(pins) {
        initKeySwitch();
    }

    /***
     * Создать ключи.
     * Предполагается, что порты будут загружены среди всех прочих настроек
     */
    MWOSKeySwitch() : MWOSKeyForSwitch<keysCount>() {
        initKeySwitch();
    }

    /***
     * Создать ключи. Удобнее при создании одного ключа.
     * @param pin порт первого ключа
     */
    MWOSKeySwitch(MWOS_PIN_INT pin) : MWOSKeyForSwitch<keysCount>() {
        initKeySwitch();
    }

    void initKeySwitch() {
        MWOSKey<keysCount>::AddParam(&p_sensorModuleId);
        MWOSKey<keysCount>::AddParam(&p_sensorIndex);
        for (uint16_t index = 0; index < keysCount; ++index) {
            _pIndex[index]=0;
        }
    }

    virtual void onInit() {
        MWOSKeyForSwitch<keysCount>::onInit();
        _sensor=NULL;
        initTimeout.start(10); // перечитаем датчики через 1 сек (что-бы они уже имели правильные показания)
    }

    void loadSensors() {
        initTimeout.stop();
        MW_LOG_MODULE(this); MW_LOG_LN(F("loadSensors..."));
        int16_t _sensor_id=MWOSModule::loadValue(-1, &p_sensorModuleId, 0);
        if (_sensor_id<2) return;
        _sensor=mwos.getModule(_sensor_id);
        if (_sensor==NULL) return;
        MWOSParam * sensorParam0=_sensor->getParam(0);
        if (sensorParam0==NULL || sensorParam0->group!=mwos_param_readonly || sensorParam0->valueType!=mwos_param_bits1) { // любой другой тип, кроме ридонли
            _sensor=NULL; // удалим не датчики
            return;
        }
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index]=MWOSModule::loadValue(0, &p_sensorIndex, index);
            if (sensorParam0->arrayCount()>_pIndex[index]) { // индекс не может быть больше длины массива параметра
                int8_t nowSensorValue=_sensor->getValue(0,_pIndex[index]);
                _lastSensorsValue.setBit(nowSensorValue,index); // прочитаем текущее значение датчика
            }
        }
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        MWOSKeyForSwitch<keysCount>::onUpdate();
        if (_sensor==NULL) {
            if (initTimeout.isStarted() && initTimeout.isTimeout()) loadSensors();
            return;
        }
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            MWOSParam * sensorParam0=_sensor->getParam(0);
            if (sensorParam0!=NULL && sensorParam0->arrayCount()>_pIndex[index]) {
                int8_t nowSensorValue=_sensor->getValue(sensorParam0,_pIndex[index]);
                if (_lastSensorsValue.getBit(index)!=nowSensorValue) {
                    MWOSKeyForSwitch<keysCount>::turn(2,index,true);
                    _lastSensorsValue.setBit(nowSensorValue,index);
                }
            }
        }
    }

};


#endif //MWOS3_MWOSKEYSWITCH_H
