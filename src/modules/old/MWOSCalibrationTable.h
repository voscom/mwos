#ifndef MWOS3_MWOSCalibrationTable_H
#define MWOS3_MWOSCalibrationTable_H

#include "../../core/MWOSModuleBase.h"
#include "../../core/MWOSLinkToValue.h"
const char MWOSCalibrationTableName[] PROGMEM = {"table"};
/***
 * Коррекция значений на множитель из таблицы
 * Можно использовать для калибровки значений
 * MWOSCalibrationTable количество записей  в таблице
 */
template<uint8_t tabLinesCount>
class MWOSCalibrationTable : public MWOSModule {
public:

#pragma pack(push,1)
    float _tabValue[tabLinesCount];
    float _tabCoefMul[tabLinesCount];
    float _tabCoefAdd[tabLinesCount];
#pragma pack(pop)

    // итоговые значения (с учетом коррекции)
    MWOS_PARAM(0, value, mwos_param_float32, mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // добавим параметры и методы линковки на значения другого параметра в другом модуле
    MWOS_PARAMS_LINK_TO_MODULE_PARAM_VALUE(1,2,3,1);
    // значения для коректировки (таблица)
    MWOS_PARAM(5, tabValue, mwos_param_float32, mwos_param_control, MWOS_STORAGE_EEPROM, tabLinesCount);
    // коэффициенты умножения, соответствующие значениям для корректировки (таблица)
    MWOS_PARAM(6, tabCoefMul, mwos_param_float32, mwos_param_control, MWOS_STORAGE_EEPROM, tabLinesCount);
    // коэффициенты умножения, соответствующие значениям для корректировки (таблица)
    MWOS_PARAM(7, tabCoefAdd, mwos_param_float32, mwos_param_control, MWOS_STORAGE_EEPROM, tabLinesCount);
    // значения таблицы c учетом коэффициента (если задавать после задания tabValue - рассчитает коэффициент)
    MWOS_PARAM(8, tabValueNeed, mwos_param_float32, mwos_param_control, MWOS_STORAGE_NO, tabLinesCount);

    MWOSCalibrationTable(char * unit_name=(char *) &MWOSCalibrationTableName) : MWOSModule(unit_name) {
        moduleType=MODULE_TABLE;
        AddParam(&p_value);
        AddParamsLinkToValue();
        AddParam(&p_tabValue);
        AddParam(&p_tabCoefMul);
        AddParam(&p_tabCoefAdd);
        AddParam(&p_tabValueNeed);
        ClearTable();
    }
    
    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (IsIdParamForLink(param->id)) return IdParamForLink(param->id,arrayIndex);
        else
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 0: {
                if (!IsParamsLinkToValue(arrayIndex)) return 0;
                double v = _linkParamValue[arrayIndex]; // значение из подлинкованного модуля,параментра,индекса
                return param->resultFloat(v * GetCorrectValue(v, (float *) &_tabCoefMul) + GetCorrectValue(v, (float *) &_tabCoefAdd));
            } break;
            case 5: return param->resultFloat(_tabValue[arrayIndex]);
            case 6: return param->resultFloat(_tabCoefMul[arrayIndex]);
            case 7: return param->resultFloat(_tabCoefAdd[arrayIndex]);
            case 8: return param->resultFloat(_tabCoefMul[arrayIndex]*_tabValue[arrayIndex]+_tabCoefAdd[arrayIndex]);
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        int16_t arrayIndex=receiverDat->array_index;

        switch (receiverDat->param_id) {
            case 8: {// tabValueNeed
                double v = receiverDat->GetValueFloat();
                if (_tabValue[arrayIndex] == 0 || arrayIndex < 0 || arrayIndex >= tabLinesCount ||
                    _tabValue[arrayIndex] > INT64_MAX) {
                    MW_LOG_MODULE(this);
                    MW_LOG(F("error! value not defined for table line: "));
                    MW_LOG(arrayIndex);
                    MW_LOG('=');
                    MW_LOG_LN(v, 4);
                    return;
                }
                // коэффициэнт = калибровочное / базовое
                _tabCoefAdd[arrayIndex] = v - _tabValue[arrayIndex] * _tabCoefMul[arrayIndex];
                MWOSModule::saveValueDouble(v, &p_tabCoefAdd, arrayIndex);  // сохраним в хранилище
                SetParamChanged(&p_tabCoefAdd, arrayIndex, true);
            } break;
            default: {
                MWOSModule::saveValueDouble(receiverDat->GetValueFloat(), getParam(receiverDat->param_id), arrayIndex);  // сохраним в хранилище
                SetParamChanged(&p_tabValueNeed,arrayIndex,true);
            }
        }
        MWOSModule::onReceiveValue(receiverDat);
    }

    /**
     * Скорректировать значение по таблицы
     * @return  Скорректированное по таблице значение
     */
    double GetCorrectValue(double value, float * _tabCoef) {
        if (_tabValue[0]>INT64_MAX) return _tabCoef[0]; // таблица пустая
        //return _tabCoef[0];
        for (uint8_t i = 0; i < tabLinesCount; i++) {
            double tabValue=_tabValue[i];
            if (value==tabValue) return _tabCoef[i];
            if (value<tabValue) {
                if (i==0) return _tabCoef[0];
                if (_tabCoef[i]<=_tabCoef[i-1]) return _tabCoef[i-1];
                double dValue=value - _tabValue[i-1];
                double deltaValues=_tabValue[i] - _tabValue[i-1];
                double deltaCoef=_tabCoef[i] - _tabCoef[i-1];
                double addCoef=deltaCoef*dValue/deltaValues;
                addCoef+=_tabCoef[i-1];
                return addCoef;
            }
        }
        return _tabCoef[0];
    }

    /**
     * Очистить таблицу
     */
    void ClearTable() {
        for (uint8_t i = 0; i < tabLinesCount; ++i) {
            _tabValue[i]= ((float) INT64_MAX)*2;
            _tabCoefMul[i]=1;
            _tabCoefAdd[i]=0;
        }
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) {
            LoadParamsAllLinkToValue();
            for (uint8_t i = 0; i < tabLinesCount; ++i) {
                _tabValue[i]=loadValueDouble(_tabValue[i], &p_tabValue, i);
                _tabCoefMul[i]=loadValueDouble(_tabCoefMul[i], &p_tabCoefMul, i);
                _tabCoefAdd[i]=loadValueDouble(_tabCoefAdd[i], &p_tabCoefAdd, i);
                SetParamChanged(&p_tabValueNeed,i,true);
            }
            SetParamChanged(&p_value,0,true);
        } else
        if (modeEvent==EVENT_UPDATE) {
            if (GetValueParamLinkToValue(0)) {
                SetParamChanged(&p_value,0,true);
            }
        }
    }


    void setDefaultParam(MWOSModuleBase * module, MWOS_PARAM_UINT paramId, MWOS_PARAM_INDEX_UINT index) {
        setDefaultParamLinkToValue(0,module,paramId,index);
    }

private:
    int64_t oldValue;
};


#endif //MWOS3_MWOSCalibrationTable_H
