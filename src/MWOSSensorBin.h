#ifndef MWOS3_MWOSSENSORBIN_H
#define MWOS3_MWOSSENSORBIN_H
/***
 * Несколько однотипных бинарных датчиков
 * датчики могут быть подключены к порту микропроцессора
 * или к платам расширителям портов через mwos.pins.add(new MWOSPins())
 *
 * ПРОВЕРЕНО!
 * с портами ESP32S3
 *
 */
#include "core/MWOSSensorBase.h"

template<uint16_t sensorsCount>
class MWOSSensorBin : public MWOSSensorBase<sensorsCount> {
public:
    uint8_t _pin[sensorsCount];

    // описание параметров
    MWOS_PARAM(1, pin, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, sensorsCount); // порты

    /***
     * Главный конструктор - создает с параметрами по умолчанию
     */
    MWOSSensorBin() : MWOSSensorBase<sensorsCount>() {
        MWOSModuleBase::AddParam(&p_pin);
        for (uint16_t i = 0; i < sensorsCount; ++i) {
            _pin[i]=255;
        }
    }

    /***
     * Создать датчики
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSSensorBin(const uint8_t * pins) : MWOSSensorBin<sensorsCount>() {
        for (uint16_t i = 0; i < sensorsCount; ++i) {
            _pin[i]=pgm_read_byte_near(pins+i);
        }
    }

    /**
     * Создать датчики (обычно используется для одиночных)
     * @param pin
     */
    MWOSSensorBin(uint8_t pin) : MWOSSensorBin<sensorsCount>() {
        _pin[0]=pin;
    }

    virtual void initSensor(int16_t index, bool pullOn) {
        MW_LOG(F("MWOSSensorBin.def pin ")); MW_LOG_LN(_pin[index]);
        MWOSSensorBase<sensorsCount>::initSensor(index, pullOn);
        _pin[index]=MWOSSensorBase<sensorsCount>::loadValue(_pin[index], &p_pin, index);
        if (pullOn || !MWOSSensorBase<sensorsCount>::_pull_off) mwos.pin(_pin[index])->mode(false, MWOSSensorBase<sensorsCount>::_sensor_pull);
        else mwos.pin(_pin[index])->mode(false, 0);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex= 0) {
        if (param->id==1) return _pin[arrayIndex];
        return MWOSSensorBase<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual bool readBoolValue(uint16_t arrayIndex) {
        return mwos.pin(_pin[arrayIndex])->readDigital();
    }


};


#endif //MWOS3_MWOSSENSORBIN_H
