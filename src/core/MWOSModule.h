#ifndef MWOS3_MWOSMODULEPARAMS_H
#define MWOS3_MWOSMODULEPARAMS_H
/***
 * Базовый класс для всех модулей контроллера
 * Модули должны наследовать этот класс
 * в конструктор необходимо добавить все параметры модуля такой строкой:
 * MWOS_PARAM(Класс,Название,тип_результата,место_хранения)
 *
 * Пример:
    MWOSController() : MWOSModule() {
        // описание параметров
        MWOS_PARAM(MWOS_controllerID,cid,mwos_param_uint32,MWOS_STORAGE_EEPROM);
        MWOS_PARAM(MWOS_sendTimeoutDSec,sendTimeoutDSec,mwos_param_uint16,MWOS_STORAGE_EEPROM);
    }
 *
 */

#include "MWOSModuleBase.h"
#include "MWOSParam.h"
#include "MWOS3.h"

extern MWOS3 mwos;

class MWOSModule: public MWOSModuleBase {
public:

    MWOSModule(char * unit_name, uint16_t unit_id=0) : MWOSModuleBase(unit_name,unit_id) {
        if (mwos.AddChild(this)) mwos.modulesCount++;
    }

    /**
     * Прочитать значение из хранилища
     * @param value
     * @return Сколько байт было прочитано
     */
    mwos_size loadValue(MWValue &value) {
        value.module_id=id;
        return mwos.loadValueFromStorage(value);
    }

    int64_t loadValueInt(const int64_t defValue, const MWOSParam &param, int16_t arrayIndex) {
        MWValue res(param.valueType,id,param.id,arrayIndex);
        res.setValueInt(defValue);
        mwos.loadValueFromStorage(res);
        return res.toInt();
    }

    double loadValueDouble(const double defValue, const MWOSParam &param, int16_t arrayIndex) {
        MWValue res(param.valueType,id,param.id,arrayIndex);
        res.setValueDouble(defValue);
        mwos.loadValueFromStorage(res);
        return res.toDouble();
    }

    String loadValueString(const String &defValue, const MWOSParam &param, uint16_t arrayIndex=0xffff) {
        MWValue res(param.valueType,id,param.id,arrayIndex);
        res.setString(defValue.c_str());
        mwos.loadValueFromStorage(res);
        return res.toString();
    }

    /**
     * Записать значение в хранилище
     * @param value
     * @return Сколько реально было записано
     */
    mwos_size saveValue(MWValue &value) {
        value.module_id=id;
        return mwos.saveValueToStorage(value);
    }

    bool saveValueInt(const int64_t newValue, const MWOSParam &param, int16_t arrayIndex) {
        MWValue res(param.valueType,id,param.id,arrayIndex);
        res.setValueInt(newValue);
        return mwos.saveValueToStorage(res)>0;
    }

    bool saveValueDouble(const double newValue, const MWOSParam &param, int16_t arrayIndex) {
        MWValue res(param.valueType,id,param.id,arrayIndex);
        res.setValueDouble(newValue);
        return mwos.saveValueToStorage(res)>0;
    }

    bool saveValueString(const String &newValue, const MWOSParam &param, int16_t arrayIndex) {
        MWValue res(param.valueType,id,param.id,arrayIndex);
        res.setString(newValue.c_str());
        return mwos.saveValueToStorage(res)>0;
    }

    /**
     * Найти модуль, по его id из параметра.
     * Если не сохранен в настройках или задан с ошибкой - использует значение по умолчанию.
     * А если и значение по умолчанию не задано, то ищет первый модуль с таким типом (кроме текущего).
     * Если итоговый модуль не соответствует сохраненному, то сохраняет полученный
     * @param defaultModule Модуль по умолчанию (если не сохранен в настройках или задан с ошибкой)
     * @param type      Тип модуля  (если MODULE_UNDEFINED, то не использует)
     * @param param     Параметр, содержащий Id модуля
     * @param arrayIndex    Индекс параметра, содержащий Id модуля
     * @return
     */
    MWOSModuleBase * loadLinkModule(MWOSModule &defaultModule, ModuleType type, MWOSParam &param, int16_t arrayIndex) {
        MWOSUnit * res=&defaultModule;
        int16_t param_id=loadValueInt(-2,param,arrayIndex);
        if (param_id>=0) {
            MWOSUnit * unit=mwos.FindChildById(param_id);
            if (unit && (type==MODULE_UNDEFINED || unit->moduleType==type)) res=unit;
        }
        if (type!=MODULE_UNDEFINED && param_id==-2 && !res) res = mwos.FindChildByModuleType(type,this);
        if (res) {
            if (res->id != param_id) {
                saveValueInt(res->id,param,arrayIndex);
                SetParamChanged(&param,arrayIndex);
            }
        } else {
            if (param_id==-2) {
                saveValueInt(-1,param,arrayIndex);
                SetParamChanged(&param,arrayIndex);
            }
        }
        return (MWOSModule *) res;
    }

    /**
     * Найти параметр по имени и сохранить в него значение из текста
     * @param str             Значение текстом (не пустая строка). Можно несколько значений через запятую. Строковое значение может быть в кавычках.
     * @param paramObj      Объект параметра
     * @param arrayIndexStr    Индекс (пустая строка - все индексы от 0)
     * @param saveValueNow    После установки значения сразу сохранить его в хранилище
     * @return
     */
    bool setValueStrToParam(const String &str, MWOSParam * paramObj, String arrayIndexStr, bool saveValueNow) {
        if (paramObj==NULL) return false;
        bool allValues;
        if (arrayIndexStr=="" || arrayIndexStr[0]>'9' || arrayIndexStr[0]<'0') {
            allValues=true;
        } else {
            allValues=false;
        }
        MWValue mwv(paramObj->valueType,id,paramObj->id,0xffff);
        if (paramObj->IsString()) {
            mwv.setString(str.c_str());
            mwv.param_index=0xffff; // все значения
            setValue(mwv);
            if (saveValueNow) mwos.saveValueToStorage(mwv);
            return true;
        }
        if (allValues && paramObj->arrayCount()>1) { // это не один параметр
            int lastPV=0;
            for (int16_t i=0; i<paramObj->arrayCount(); i++) {
                String vi;
                int pV=str.indexOf(',',lastPV);
                if (pV>=0) {
                    vi=str.substring(lastPV,pV);
                } else {
                    vi=str.substring(lastPV);
                }
                mwv.parseString(vi.c_str());
                setValue(mwv);
                if (saveValueNow) mwos.saveValueToStorage(mwv);
                if (pV<0) break;
                else lastPV=pV+1;
            }
        } else {
            int16_t arrayIndexInt=arrayIndexStr.toInt();
            if (arrayIndexInt<0 || arrayIndexInt>paramObj->arrayCount())
            mwv.param_index=arrayIndexInt;
            mwv.parseString(str.c_str());
            setValue(mwv);
            if (saveValueNow) mwos.saveValueToStorage(mwv);
            //MW_LOG_MODULE(this,paramObj,arrayIndexInt); MW_LOG(F("value = ")); MW_LOG_LN(v);
        }
        return true;
    }

    /**
     * Получить текстовое значения параметра (или список значений параметра через запятую)
    * @param param      Объект параметра
     * @param arrayIndex    Индекс (пустая строка - все индексы от 0)
     * @return Значения параметра
     */
    String getValueStrFromParam(MWOSParam * param, const String &arrayIndex) {
        if (!param) return "";
        int16_t arrayIndexInt=0;
        bool allValues;
        if (arrayIndex=="" || arrayIndex[0]>'9' || arrayIndex[0]<'0') {
            allValues=true;
        } else {
            allValues=false;
            arrayIndexInt=arrayIndex.toInt();
        }
        MWValue v(param->valueType,id,param->id,arrayIndexInt);
        onEvent(EVENT_GET_VALUE,v);
        if (param->IsString()) {
            v.param_index=0xffff; // все значения
            return '"'+v.toString()+'"';
        }
        String result=v.toString();
        if (allValues && param->arrayCount()>1) {
            // это не один параметр
            for (int16_t i=1; i<param->arrayCount(); i++) {
                v.param_index=i;
                onEvent(EVENT_GET_VALUE,v);
                result+=", "+v.toString();
            }
        }
        return result;
    }

    /***
     * Помечает значение, как измененное для отправки.
     * При необходимости - сохраняет в хранилище.
     * Пины проверяет на ошибки (занятые пины).
     * Вызывается при получении нового значения от сервера
     */
    void onReceiveValue(MWValue &data) {
        MWOSParam * param=getParam(data.param_id);
        if (param) {
            MW_LOG_MODULE(this, param, data.param_index); MW_LOG(F("onReceiveValue = ")); MW_LOG_LN(data.toString());
            SetParamChanged(param, data.param_index, true);
            if (data.type == PARAM_AUTO) data.type = param->valueType;
            if (!param->IsGroup(PARAM_TYPE_READONLY)) { // параметр не имеет флага readonly
#ifndef MWOS_GLOBAL_NO_PIN
                if (param->IsGroup(PARAM_TYPE_PIN)) { // для пинов, проверим что этот пин не занят
                    MWOS_PIN_INT pin= (MWOS_PIN_INT) data.toInt();
                    if (mwos.FindByPin(pin)) {
                        pin=-2; // этот пин уже занят - запишем (-2 - ошибка)
                        data.setValueInt(pin);
                    }
                }
#endif
                if (mwos.saveValueToStorage(data)>0) {  // сохраним в хранилище

                }
            }
            onEvent(EVENT_CHANGE,data); // после изменения настроек или пинов - вызовем событие EVENT_CHANGE
        }
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
        switch (modeEvent) {
            case EVENT_SET_VALUE: onReceiveValue(data); break;
            case EVENT_GET_VALUE: loadValue(data); break;
        }
    }


};


#endif
