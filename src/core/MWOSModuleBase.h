#ifndef MWOS3_MWMODULEBASE_H
#define MWOS3_MWMODULEBASE_H
/***
 * Базовый класс модулей
 * Модули модут перехватывать следующие события
 *
 * onEvent() - вызывается на кождое событие и каждый тик системы. модуль сам должен следить, чтобы как можно быстрее обработать это событие
 *
 * int64_t getValue(uint16_t paramNum, int64_t value, int16_t arrayIndex) - запрошено значение параметра.
 * Вызывается после чтения значения параметра из хранилища. Можно изменить отправляемое значение.
 *
 * void onReceiveValue(MWOSNetReceiverFields * receiverDat) - принято значение параметра.
 * Вызывается перед сохранением значения в хранилище (если оно задано). Можно изменить сохраняемое значение.
 *
 *
 */

#include <core/MWValue.h>
#include "MWOSParam.h"
#include "MWOSUnit.h"

// типы параметров
#include "MWOSParent.h"
#include "core/adlib/MWBitsMask.h"

int64_t MWOSModuleBase_return_result_value=0;

/***
 * Интерфейс модуля
 */
class MWOSModuleBase : public MWOSParent {
public:

    MWOSModuleBase(char * unit_name, uint16_t unit_id=0) : MWOSParent(unit_name,unit_id) {
        unitType=MODULE;
    }

    /**
     * Добавить модулю новый параметр
     * @param param
     */
    bool AddParam(const MWOSParam &param) {
        bool res=AddChild((MWOSUnit *) &param);
        return res;
    }

    MWOSParam * getParam(uint16_t paramNum) {
        return (MWOSParam *) MWOSParent::FindChildById(paramNum);
    }

    /***
     * Установить/снять признак необходимости отправки значения этого параметра на сервер
     * @param param_id  id параметра (если UINT16_MAX - все параметры)
     * @param arrayIndex Не используется!
     * @param changedV   0=снять, 1=Установить, 2=установить для всех, у кого установлено sended или changed (только для param_id=UINT16_MAX)
     * @param onlyControl  Только для параметров управления
     * @return  Количество измененных параметров
     */
    uint16_t SetParamChangedByParamId(uint16_t param_id, uint16_t arrayIndex=UINT16_MAX, uint8_t changedV=1, bool onlyControl=false) {
        MWOSParam * paramNow=(MWOSParam *) child;
        bool changedNow=changedV>0;
        uint16_t res=0;
        while (paramNow && paramNow->unitType==UnitType::PARAM) {
            if (changedV>1) changedNow=paramNow->sended || paramNow->changed;
            if (paramNow->id==param_id) {
                if (paramNow->changed!=changedNow) {
                    paramNow->changed=changedNow;
                    paramNow->sended=!changedNow;
                    res++;
                }
                return res;
            } else
            if (param_id==UINT16_MAX && (!onlyControl || paramNow->IsParamControl())) {
                if (paramNow->changed!=changedNow) {
                    paramNow->changed=changedNow;
                    paramNow->sended=!changedNow;
                    res++;
                }
            }
            paramNow=(MWOSParam *) paramNow->next;
        }
        return res;
    }

    /**
    * Установить/снять признак необходимости отправки формата этого параметра на сервер.
     * @param sendInfNow  0=снять, 1=Установить
     * @param param_id  id параметра (если UINT16_MAX - все параметры)
     * @return
     */
    uint16_t SetParamSendInfByParamId(bool sendInfNow, uint16_t param_id=UINT16_MAX) {
        MWOSParam * paramNow=(MWOSParam *) child;
        uint16_t res=0;
        while (paramNow && paramNow->unitType==UnitType::PARAM) {
            if (paramNow->id==param_id || param_id==UINT16_MAX) {
                paramNow->sendInf=sendInfNow;
                res++;
            }
            paramNow=(MWOSParam *) paramNow->next;
        }
        return res;
    }

    /**
     * Установить/снять признак необходимости отправки этого параметра на сервер
     * @param param         Параметр (или nullptr - все параметры)
     * @param arrayIndex Не используется!
     * @param changed   Установить/снять
     */
    void SetParamChanged(MWOSParam * param, uint16_t arrayIndex=UINT16_MAX, bool changed=true) {
        if (!param) SetParamChangedByParamId(UINT16_MAX,changed);
        else {
            if ( param->changed!=changed) {
                param->sended=!changed;
                param->changed=changed;
            }
        }
    }

    /**
     * Необходима отправка значения этого параметра на сервер?
     * @param param_id id параметра
     * @param arrayIndex Не используется!
     * @return  true - необходима отправка
     */
    bool IsParamChangedByParamId(uint16_t param_id, uint16_t arrayIndex=0) {
        return IsParamChanged((MWOSParam *) FindChildById(param_id));
    }

    /**
     * Необходима отправка значения этого параметра на сервер?
     * @param param параметр
     * @param arrayIndex Не используется!
     * @return  true - необходима отправка
     */
    bool IsParamChanged(MWOSParam * param, uint16_t arrayIndex=0) {
        if (param) return param->changed;
        return false;
    }

    bool IsParamSendedByParamId(uint16_t param_id, uint16_t arrayIndex=0) {
        return IsParamSended((MWOSParam *) FindChildById(param_id));
    }

    bool IsParamSended(MWOSParam * param) {
        if (param) return param->sended;
        return false;
    }

    /***
     * Ищет параметр, где используется этот пин
     * @param pinNum    номер пина
     * @return  параметр или nullptr
     */
    MWOSParam * FindByPin(MWOS_PIN_INT pinNum) {
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow && paramNow->unitType==UnitType::PARAM) {
            if (paramNow->IsGroup(PARAM_TYPE_PIN)) {
                for (MWOS_PARAM_INDEX_UINT i = 0; i < paramNow->arrayCount(); i++) {
                    if (getValue((int64_t) -1,paramNow,i)==pinNum) return paramNow;
                }
            }
            paramNow=(MWOSParam *) paramNow->next;
        }
        return nullptr;
    }

     /**
      * Статистика по параметрам
      * @param storageType  Количество всех значений параметров модуля (false - просто количество параметров модуля)
      * @return     Общее количество всех значений всех параметров этого модуля
      */
    uint32_t GetParamCount(bool getValuesCount) {
        uint32_t res=0;
        MWOSParam * param=(MWOSParam *) child;
        while (param && param->unitType==UnitType::PARAM) {
            if (getValuesCount) res+=param->indexCount();
            else res++;
            param=(MWOSParam *) param->next;
        }
        return res;
    }

    /***
     * Общий битовый размер всех параметров модуля для заданного типа хранилища
      * @param storageType  По хранилищам
      * @param IsParamBit   Отдельно для битовых и для байтовых параметров
      * @return
      */
    int32_t paramsBitsSize(int8_t storageType, bool IsParamBit) {
        int32_t bitSize=0;
        MWOSParam * param=(MWOSParam *) child;
        while (param && param->unitType==UnitType::PARAM) {
            if (param->IsStorage(storageType) && (param->IsBit()==IsParamBit)) {
                bitSize+=param->bitsSize(true);
            }
            param=(MWOSParam *) param->next;
        }
        return bitSize;
    }

    /**
     * Рассчитать битовое смещение параметра для этого типа хранилища. От начала хранилища для этого модуля.
     * Смещение битовых параметров рассчитываются отдельно.
     * @param param             Параметр
     * @param moduleBitOffset   Битовое смещение параметра, относительно модуля
     * @return
     */
    int32_t getParamBitsOffset(MWOSParam * param) {
        int32_t bitOffset=0;
        int8_t storageType=param->getStorage();
        bool IsParamBit=param->IsBit();
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow && paramNow->unitType==UnitType::PARAM) {
            if (paramNow->IsStorage(storageType) && (paramNow->IsBit()==IsParamBit)) { // отдельно по хранилищам и отдельно битовые параметры
                if (param==paramNow) return bitOffset;
                bitOffset+=paramNow->bitsSize(true);
            }
            paramNow=(MWOSParam *) paramNow->next;
        }
        return 0;
    }

    void setValueByParamId(const String &v, MWOS_PARAM_UINT paramId, int16_t arrayIndex) {
        setValue(v,getParam(paramId),arrayIndex);
    }

    void setValueByParamId(const double v, MWOS_PARAM_UINT paramId, int16_t arrayIndex) {
        setValue(v,getParam(paramId),arrayIndex);
    }

    void setValueByParamId(const int64_t v, MWOS_PARAM_UINT paramId, int16_t arrayIndex) {
        setValue(v,getParam(paramId),arrayIndex);
    }

    void setValue(MWValue &data) {
        onEvent(EVENT_SET_VALUE,data);
    }

    /***
     * Установить значение и вызвать для него onReceiveValue
     * @param v
     * @param param
     * @param arrayIndex
     */
    void setValue(const int64_t v, MWOSParam * param, int16_t arrayIndex) {
        if (!param) return;
        MWValue data;
        data.setValueInt(v,param->valueType);
        data.param_id=param->id;
        data.module_id=id;
        data.param_index=arrayIndex;
        onEvent(EVENT_SET_VALUE,data);
    }

    void setValue(const String &vs, MWOSParam * param, int16_t arrayIndex) {
        if (!param) return;
        MWValue data;
        data.setString(vs.c_str());
        data.param_id=param->id;
        data.module_id=id;
        data.param_index=arrayIndex;
        onEvent(EVENT_SET_VALUE,data);
    }

    void setValue(const double vf, MWOSParam * param, int16_t arrayIndex) {
        if (!param) return;
        MWValue data;
        data.setValueDouble(vf,param->valueType);
        data.param_id=param->id;
        data.module_id=id;
        data.param_index=arrayIndex;
        onEvent(EVENT_SET_VALUE,data);
    }

    String getValueByParamId(const String &def, MWOS_PARAM_UINT paramId, int16_t arrayIndex) {
        return getValue(def,getParam(paramId),arrayIndex);
    }

    double getValueByParamId(const double def, MWOS_PARAM_UINT paramId, int16_t arrayIndex) {
        return getValue(def,getParam(paramId),arrayIndex);
    }

    int64_t getValueByParamId(const int64_t def, MWOS_PARAM_UINT paramId, int16_t arrayIndex) {
        return getValue(def,getParam(paramId),arrayIndex);
    }

    /**
     * Запросить значение параметра в data. Адрес тоже в data.
     * @param data      Адрес параметра и место для возврата значения.
     * @param param     Полезно для оптимизации. Но если не передавать, то найдет само.
     */
    void getValue(MWValue &data, MWOSParam * param=nullptr) {
        if (!param) {
            if (data.param_id==UINT16_MAX) return;
            param=getParam(data.param_id);
            if (!param) return;
        } else data.param_id=param->id;
        data.type=param->valueType;
        data.module_id=id;
        onEvent(EVENT_GET_VALUE,data);
    }

    String getValue(const String &def, MWOSParam * param, int16_t arrayIndex) {
        if (!param) return def;
        MWValue data;
        data.setString(def.c_str());
        data.module_id=this->id;
        data.param_id=param->id;
        data.param_index=arrayIndex;
        onEvent(EVENT_GET_VALUE,data);
        return data.toString();
    }

    double getValue(const double def, MWOSParam * param, int16_t arrayIndex) {
        if (!param) return def;
        MWValue data;
        data.setValueDouble(def,param->valueType);
        data.module_id=this->id;
        data.param_id=param->id;
        data.param_index=arrayIndex;
        onEvent(EVENT_GET_VALUE,data);
        return data.toDouble();
    }

    int64_t getValue(const int64_t def, MWOSParam * param, int16_t arrayIndex) {
        if (!param) return def;
        MWValue data;
        data.setValueInt(def,param->valueType);
        data.module_id=this->id;
        data.param_id=param->id;
        data.param_index=arrayIndex;
        onEvent(EVENT_GET_VALUE,data);
        return data.toInt();
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
    }

};


#endif //MWOS3_MWMODULEBASE_H
