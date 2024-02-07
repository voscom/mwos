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

template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorBin : public MWOSSensorBase<sensorsCount> {
public:
    MWOS_PIN_INT _pin[sensorsCount];

    // описание параметров
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, mwos_param_storage_eeprom, sensorsCount); // порты

    /***
     * Главный конструктор - создает с параметрами по умолчанию
     */
    MWOSSensorBin() : MWOSSensorBase<sensorsCount>() {
        MWOSModuleBase::AddParam(&p_pin);
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i]=-1;
        }
    }

    /***
     * Создать датчики
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSSensorBin(const uint8_t * pins) : MWOSSensorBin<sensorsCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _pin[i]=pgm_read_byte_near(pins+i);
        }
    }

    /**
     * Создать датчики (обычно используется для одиночных)
     * @param pin
     */
    MWOSSensorBin(MWOS_PIN_INT pin) : MWOSSensorBin<sensorsCount>() {
        _pin[0]=pin;
    }

    void initSensor(int16_t index, bool pullOn) {
        MWOSSensorBase<sensorsCount>::initSensor(index, pullOn);
        MW_LOG(F("pin ")); MW_LOG(index); MW_LOG('>');
        _pin[index]=MWOSSensorBase<sensorsCount>::loadValue(_pin[index], &p_pin, index);
        if (_pin[index]<0) return;
        //MW_LOG_MODULE(this); MW_LOG(F("MWOSSensorBin.def pin ")); MW_LOG(index); MW_LOG('='); MW_LOG_LN(_pin[index]);
        if (pullOn || !MWOSSensorBase<sensorsCount>::_pull_off) mwos.pin(_pin[index])->mode(false, MWOSSensorBase<sensorsCount>::_sensor_pull);
        else mwos.pin(_pin[index])->mode(false, 0);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==1) return _pin[arrayIndex];
        return MWOSSensorBase<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual bool readBoolValue(uint16_t arrayIndex) {
        if (_pin[arrayIndex]<0) return 0;
        bool res=mwos.pin(_pin[arrayIndex])->readDigital();
        //MW_LOG_MODULE(this); MW_LOG(F("Get sensor ")); MW_LOG(arrayIndex); MW_LOG('='); MW_LOG_LN(res);
        return res;
    }


};


#endif //MWOS3_MWOSSENSORBIN_H
