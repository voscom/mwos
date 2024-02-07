#ifndef MWOS3_MWOSSENSORSTREAM_H
#define MWOS3_MWOSSENSORSTREAM_H
/***
 * Несколько однотипных аналоговых датчиков
 * показания датчика считываются с последовательного порта
 */
#include "core/MWOSSensorAnalog.h"

/**
 * Количество контактов для подключенного устройства
 */
#define MWOSSensorStreamPinsCount 2

template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorStream : public MWOSSensorAnalog<sensorsCount> {
public:
    Stream * _stream;
    MWOS_PIN_INT _pin[MWOSSensorStreamPinsCount];

    //************************ описание параметров ***********************/
    // порты, используемые для получения и отправки данных
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin+mwos_param_readonly, mwos_param_storage_no, MWOSSensorStreamPinsCount);

    MWOSSensorStream(Stream * stream) : MWOSSensorAnalog<sensorsCount>() {
        MWOSModuleBase::AddParam(&p_pin);
        _stream=stream;
        for (MWOS_PARAM_INDEX_UINT i = 0; i < MWOSSensorStreamPinsCount; ++i) {
            _pin[i]=-1;
        }
    }

     virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==1) return _pin[arrayIndex];
        return MWOSSensorAnalog<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    char separator = ','; // separator you described

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        while (_stream->available()>0) {
            uint8_t d = _stream->read();
            MW_LOG(d); // TODO: пока выводим в терминал, как есть
        }

    }

    virtual void initSensor(int16_t index, bool pullOn) {
         MWOSSensorAnalog<sensorsCount>::initSensor(index, pullOn);
    }

    /**
     * Опросить аналоговый датчик для получения новых показаний
     */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        return 0;
    }


};


#endif //MWOS3_MWOSSENSORADC_H
