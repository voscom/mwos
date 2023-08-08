#ifndef MWOS3_MWOSKEYEXT_H
#define MWOS3_MWOSKEYEXT_H
#include "MWOSKey.h"
extern MWOS3 mwos;
/***
 * Ключи с расширенной функциональностью
 * Может сам выключасть и/или включаться через таймаут
 * Может включаться ежедневно в заданное время (минут с полуночи). В эту минуту суток принудительно включает ключ, если он выключен
 * Если эта возможность не нужна - рекомендуется использовать MWOSKey
 */
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKeyExt : public MWOSKey<keysCount> {
public:
#pragma pack(push,1)
    uint16_t _timeon[keysCount];
    uint16_t _timeoff[keysCount];
    uint16_t _scheduleOn[keysCount];
    MWTimeout lastTimeOut[keysCount];
    uint16_t nextKeyIndex=0;
#pragma pack(pop)

    // ************ описание параметров *****************

    // максимальное время включения ключа [сек/10]
    MWOS_PARAM(7, timeon, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);
    // максимальное время выключения ключа [сек/10]
    MWOS_PARAM(8, timeoff, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);
    // в какое время включать ключ ежедневно (минут с полуночи, если не 0)
    MWOS_PARAM(9, scheduleOn, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, keysCount);

    /***
     * Создать ключи
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию
     */
    MWOSKeyExt(const uint8_t * pins) : MWOSKey<keysCount>(pins) {
        initKeyExt();
    }

    /***
     * Создать ключи.
     * Предполагается, что порты будут загружены среди всех прочих настроек
     */
    MWOSKeyExt() : MWOSKey<keysCount>() {
        initKeyExt();
    }

    /***
     * Создать ключи. Удобнее при создании одного ключа.
     * @param pin порт первого ключа
     */
    MWOSKeyExt(MWOS_PIN_INT pin) : MWOSKey<keysCount>(pin) {
        initKeyExt();
    }

    void initKeyExt() {
        MWOSKey<keysCount>::AddParam(&p_timeon);
        MWOSKey<keysCount>::AddParam(&p_timeoff);
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _timeon[index]=0;
            _timeoff[index]=0;
            _scheduleOn[index]=0;
        }
    }

    virtual void onInit() {
        MW_LOG_LN(F("MWOSKeyExt.onInit"));
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _timeon[index]=MWOSModule::loadValue(_timeon[index], &p_timeon, index);
            _timeoff[index]=MWOSModule::loadValue(_timeoff[index], &p_timeoff, index);
            _scheduleOn[index]=MWOSModule::loadValue(_scheduleOn[index], &p_scheduleOn, index);
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
        if (_scheduleOn[nextKeyIndex]>0 && mwos.timeModule!=NULL && ((MWOSTime *) mwos.timeModule)->dailyMin()==_scheduleOn[nextKeyIndex]) turn(1, nextKeyIndex, true); // включим по расписанию
        nextKeyIndex++;
        if (nextKeyIndex >= keysCount) nextKeyIndex=0;
    }

    virtual void turn(uint8_t mode, int16_t index=0, bool saveToStorage=true) {
        MWOSKey<keysCount>::turn(mode,index,saveToStorage);
        if (MWOSKey<keysCount>::_turn.getBit(index)>0) {
            if (_timeon[index] > 0) {
                lastTimeOut[index].start(_timeon[index]);
                MW_LOG_MODULE(this,index); MW_LOG(F("timeout on = ")); MW_LOG_LN(_timeon[index]);
            }
        } else {
            if (_timeoff[index] > 0) {
                lastTimeOut[index].start(_timeoff[index]);
                MW_LOG_MODULE(this,index); MW_LOG(F("timeout off = ")); MW_LOG_LN(_timeoff[index]);
            }
        }

    }

};


#endif //MWOS3_MWOSKEY_H
