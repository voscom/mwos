#ifndef MWOS3_MWOSKEYEXT_H
#define MWOS3_MWOSKEYEXT_H
#include "MWOSKey.h"
/***
 * Ключи с расширенной функциональностью
 * Может сам выключасть и/или включаться через таймаут
 *
 */
template<uint16_t keysCount>
class MWOSKeyExt : public MWOSKey<keysCount> {
public:
#pragma pack(push,1)
    uint16_t _timeon[keysCount];
    uint16_t _timeoff[keysCount];
    MWTimeout lastTimeOut[keysCount];
    uint16_t nextKeyIndex=0;
#pragma pack(pop)

    // ************ описание параметров *****************

    // максимальное время включения ключа [сек/10]
    MWOS_PARAM(6, timeon, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);
    // максимальное время выключения ключа [сек/10]
    MWOS_PARAM(7, timeoff, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);

    /***
     * Создать ключи
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSKeyExt(const uint8_t * pins) : MWOSKey<keysCount>(pins) {
        afterCreate();
    }

    /***
     * Создать ключи.
     * Предполагается, что порты будут загружены среди всех прочих настроек
     */
    MWOSKeyExt() : MWOSKey<keysCount>() {
    }

    /***
     * Создать ключи. Удобнее при создании одного ключа.
     * @param pin порт первого ключа
     */
    MWOSKeyExt(uint8_t pin) : MWOSKey<keysCount>(pin) {
        afterCreate();
    }

    void afterCreate() {
        MWOSKey<keysCount>::AddParam(&p_timeon);
        MWOSKey<keysCount>::AddParam(&p_timeoff);
        for (uint16_t index = 0; index < keysCount; ++index) {
            _timeon[index]=0;
            _timeoff[index]=0;
        }
    }

    virtual void onInit() {
        MW_LOG_LN(F("MWOSKeyExt.onInit"));
        for (uint16_t index = 0; index < keysCount; ++index) {
            _timeon[index]=MWOSModule::loadValue(_timeon[index], &p_timeon, index);
            _timeoff[index]=MWOSModule::loadValue(_timeoff[index], &p_timeoff, index);
            MW_LOG(F("timeoff")); MW_LOG(index); MW_LOG('='); MW_LOG_LN(_timeoff[index]);
        }
        MWOSKey<keysCount>::onInit();
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (MWOSKey<keysCount>::getValueBool(nextKeyIndex)>0) {
            if ((_timeon[nextKeyIndex] > 0) && (lastTimeOut[nextKeyIndex].isTimeout())) turn(0, nextKeyIndex, true);
        } else {
            if ((_timeoff[nextKeyIndex] > 0) && (lastTimeOut[nextKeyIndex].isTimeout())) turn(1, nextKeyIndex, true);
        }
        nextKeyIndex++;
        if (nextKeyIndex >= keysCount) nextKeyIndex=0;
    }

    virtual void turn(uint8_t mode, int16_t index, bool saveToStorage) {
        MWOSKey<keysCount>::turn(mode,index,saveToStorage);
        if (MWOSKey<keysCount>::_value.getBit(index)>0) {
            if (_timeon[index] > 0) {
                lastTimeOut[index].start(_timeon[index]);
                MW_LOG_MODULE(this); MW_LOG(F("timeout on = ")); MW_LOG_LN(_timeon[index]);
            }
        } else {
            if (_timeoff[index] > 0) {
                lastTimeOut[index].start(_timeoff[index]);
                MW_LOG_MODULE(this); MW_LOG(F("timeout off = ")); MW_LOG_LN(_timeoff[index]);
            }
        }

    }

};


#endif //MWOS3_MWOSKEY_H
