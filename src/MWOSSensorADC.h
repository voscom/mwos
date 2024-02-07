#ifndef MWOS3_MWOSSENSORADC_H
#define MWOS3_MWOSSENSORADC_H
/***
 * Несколько однотипных аналоговых датчиков
 * датчики могут быть подключены к ADC-порту микропроцессора
 * или к платам расширителям портов через mwos.pins.add(new MWOSPins())
 */
#include "core/MWOSSensorAnalog.h"

template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorADC : public MWOSSensorAnalog<sensorsCount> {
public:
    MWOS_PIN_INT _pin[sensorsCount];

    //************************ описание параметров ***********************/
    // порты
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, mwos_param_storage_eeprom, sensorsCount);

    MWOSSensorADC() : MWOSSensorAnalog<sensorsCount>() {
        MWOSModuleBase::setName((char *) F("adc"));
        MWOSModuleBase::AddParam(&p_pin);
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i]=-1;
        }
    }

    /***
     * Создать датчики
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSSensorADC(const uint8_t * pins) : MWOSSensorADC<sensorsCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i]=pgm_read_byte_near(pins+i);
        }
    }

    MWOSSensorADC(MWOS_PIN_INT pin) : MWOSSensorADC<sensorsCount>() {
        _pin[0]=pin;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==1) return _pin[arrayIndex];
        return MWOSSensorAnalog<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual void initSensor(int16_t index, bool pullOn) {
         MWOSSensorAnalog<sensorsCount>::initSensor(index, pullOn);
        _pin[index]=MWOSSensorAnalog<sensorsCount>::loadValue(_pin[index], &p_pin, index);
        if (_pin[index]<0) return;
        if (pullOn || !MWOSSensorAnalog<sensorsCount>::_pull_off) mwos.pin(_pin[index])->mode(false, MWOSSensorAnalog<sensorsCount>::_sensor_pull);
        else mwos.pin(_pin[index])->mode(false, 0);
    }

    /**
     * Опросить аналоговый датчик для получения новых показаний
     */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        if (_pin[arrayIndex]<0) return 0;
        return mwos.pin(_pin[arrayIndex])->readAnalog();
    }

    virtual int32_t readDigitalValue(int16_t arrayIndex) {
        if (_pin[arrayIndex]<0) return 0;
        return mwos.pin(_pin[arrayIndex])->readDigital();
    }


};


#endif //MWOS3_MWOSSENSORADC_H
