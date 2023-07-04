#ifndef MWOS3_MWOSKEYSWITCH_H
#define MWOS3_MWOSKEYSWITCH_H
/***
 * Переключатель (ключ, переключаемый датчиком)
 * Управляется заданным датчиком. При изменении бинарного значения датчика - переключает ключ.
 */

#include "MWOSKey.h"
#include "core/MWOSSensorBase.h"

template<uint16_t keysCount>
class MWOSKeySwitch : public MWOSKeyExt<keysCount> {
public:
#pragma pack(push,1)
    uint8_t _last_sensor_bin[keysCount];
    MWOSModule * _sensor[keysCount];
#pragma pack(pop)


    // ************ описание параметров *****************
    // датчик для управления этим переключателем
    MWOS_PARAM(10, sensor, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);

    /***
     * Создать ключи
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSKeySwitch(const uint8_t * pins) : MWOSKeyExt<keysCount>(pins) {
    }

    /***
     * Создать ключи.
     * Предполагается, что порты будут загружены среди всех прочих настроек
     */
    MWOSKeySwitch() : MWOSKeyExt<keysCount>() {
    }

    /***
     * Создать ключи. Удобнее при создании одного ключа.
     * @param pin порт первого ключа
     */
    MWOSKeySwitch(uint8_t pin) : MWOSKeyExt<keysCount>(pin) {
    }

    virtual char * getName() {  return (char *) F("switch"); }

    virtual uint16_t getParamsCount() {
        return 9;
    }
    virtual MWOSParam * getParam(uint16_t paramNum) {
        switch (paramNum) {
            case 8: return &p_sensor;
        }
        return MWOSKeyExt<keysCount>::getParam(paramNum);
    }

    virtual void onInit() {
        MWOSKeyExt<keysCount>::onInit();
        for (uint16_t index = 0; index < keysCount; ++index) {
            _last_sensor_bin[index]=255;
            _sensor[index]=mwos.getModule(loadValue(UINT16_MAX, &p_sensor, index));
            if (_sensor[index]!=NULL) {
                MWOSParam * sensorParam0=_sensor[index]->getParam(0);
                if (sensorParam0==NULL || !sensorParam0->IsGroup(mwos_param_sensor)) _sensor[index]=NULL; // удалим не датчики
            }
        }
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        MWOSKeyExt<keysCount>::onUpdate();
        for (uint16_t index = 0; index < keysCount; ++index) if (_sensor[index]!=NULL) {
            uint8_t nowSensorValue=_sensor[index]->getValue(0);
            if (_last_sensor_bin[index]!=nowSensorValue) {
                MWOSKeyExt<keysCount>::turn(2,index,true);
                _last_sensor_bin[index]!=nowSensorValue;
            }
        }
    }

};


#endif //MWOS3_MWOSKEYSWITCH_H
