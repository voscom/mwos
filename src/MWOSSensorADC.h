#ifndef MWOS3_MWOSSENSORADC_H
#define MWOS3_MWOSSENSORADC_H
/***
 * Несколько однотипных аналоговых датчиков
 * датчики могут быть подключены к ADC-порту микропроцессора
 * или к платам расширителям портов через mwos.pins.add(new MWOSPins())
 */
#include "core/MWOSSensorAnalog.h"

template<uint16_t sensorsCount>
class MWOSSensorADC : public MWOSSensorAnalog<sensorsCount> {
public:
    uint8_t _pin[sensorsCount];

    //************************ описание параметров ***********************/
    // порты
    MWOS_PARAM(1, pin, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, sensorsCount);

    MWOSSensorADC() : MWOSSensorAnalog<sensorsCount>() {
        MWOSModuleBase::setName((char *) F("adc"));
        MWOSModuleBase::AddParam(&p_pin);
        for (uint16_t i = 0; i < sensorsCount; ++i) {
            _pin[i]=255;
        }
    }

    /***
     * Создать датчики
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSSensorADC(const uint8_t * pins) : MWOSSensorADC<sensorsCount>() {
        for (uint16_t i = 0; i < sensorsCount; ++i) {
            _pin[i]=pgm_read_byte_near(pins+i);
        }
    }

    MWOSSensorADC(uint8_t pin) : MWOSSensorADC<sensorsCount>() {
        _pin[0]=pin;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex= 0) {
        if (param->id==1) return _pin[arrayIndex];
        return MWOSSensorAnalog<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual void initSensor(int16_t index, bool pullOn) {
         MWOSSensorAnalog<sensorsCount>::initSensor(index, pullOn);
        _pin[index]=MWOSSensorAnalog<sensorsCount>::loadValue(_pin[index], &p_pin, index);
        if (pullOn || !MWOSSensorAnalog<sensorsCount>::_pull_off) mwos.pin(_pin[index])->mode(false, MWOSSensorAnalog<sensorsCount>::_sensor_pull);
        else mwos.pin(_pin[index])->mode(false, 0);
    }

    /**
     * Опросить аналоговый датчик для получения новых показаний
     */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        return mwos.pin(_pin[arrayIndex])->readAnalog();
    }

    virtual int32_t readDigitalValue(int16_t arrayIndex) {
        return mwos.pin(_pin[arrayIndex])->readDigital();
    }


};


#endif //MWOS3_MWOSSENSORADC_H
