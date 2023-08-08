#ifndef MWOS3_MWOSKEYSWITCH_H
#define MWOS3_MWOSKEYSWITCH_H
/***
 * Переключатель (ключ, переключаемый датчиком)
 * Управляется заданным датчиком. При изменении бинарного значения датчика - переключает ключ.
 */

#include "MWOSKeyExt.h"
#include "core/MWOSSensorBase.h"

template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKeySwitch : public MWOSKeyExt<keysCount> {
public:
#pragma pack(push,1)
    MWOSModuleBase * _sensor;
    MWOS_PARAM_INDEX_UINT _pIndex[keysCount];
    MWBitsMaskStat<keysCount> _lastSensorsValue;
    //int8_t _last_sensor_bin[keysCount]; // последние бинарные значения
#pragma pack(pop)


    // ************ описание параметров *****************
    // датчик для управления этим переключателем
    MWOS_PARAM(10, sensorModuleId, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // индексы параметра 0 этого датчика (если там много однотипных датчиков, то каждый из этих датчиков может управлять своим ключом в этом модуле)
    MWOS_PARAM(11, sensorIndex, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);

    /***
     * Создать ключи
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSKeySwitch(const uint8_t * pins) : MWOSKeyExt<keysCount>(pins) {
        initKeySwitch();
    }

    /***
     * Создать ключи.
     * Предполагается, что порты будут загружены среди всех прочих настроек
     */
    MWOSKeySwitch() : MWOSKeyExt<keysCount>() {
        initKeySwitch();
    }

    /***
     * Создать ключи. Удобнее при создании одного ключа.
     * @param pin порт первого ключа
     */
    MWOSKeySwitch(MWOS_PIN_INT pin) : MWOSKeyExt<keysCount>() {
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
        MWOSKeyExt<keysCount>::onInit();
        _sensor=mwos.getModule(MWOSModule::loadValue(-1, &p_sensorModuleId, 0));
        if (_sensor==NULL) return;
        MWOSParam * sensorParam0=_sensor->getParam(0);
        if (sensorParam0==NULL || !sensorParam0->IsGroup(mwos_param_sensor)) {
            _sensor=NULL; // удалим не датчики
            return;
        }
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pIndex[index]=MWOSModule::loadValue(0, &p_sensorIndex, index);
            if (sensorParam0->arrayCount()<=_pIndex[index]) _pIndex[index]=0; // индекс не может быть больше длины массива параметра
            _lastSensorsValue.setBit(_sensor->getValue(0,_pIndex[index]),index); // прочитаем текущее значение датчика
        }

    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        MWOSKeyExt<keysCount>::onUpdate();
        if (_sensor==NULL) return;
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index)  {
            int8_t nowSensorValue=_sensor->getValue(0,_pIndex[index]);
            if (_lastSensorsValue.getBit(index)!=nowSensorValue) {
                MWOSKeyExt<keysCount>::turn(2,index,true);
                _lastSensorsValue.setBit(nowSensorValue,index);
            }
        }
    }

};


#endif //MWOS3_MWOSKEYSWITCH_H
