#ifndef MWOS3_MWMODULEBASE_H
#define MWOS3_MWMODULEBASE_H
/***
 * Базовый класс модулей
 * Модули модут перехватывать следующие события
 *
 * onInit() - инициализация системы, после старта или глубокого сна (все модули уже созданы)
 *
 * onUpdate() - вызывается каждый тик системы. модуль сам должен следить, чтобы как можно быстрее обработать это событие
 *
 * onReciveStart(uint16_t paramNum, uint16_t blockSize) - начало приема блока данных для параметра (типа байтового массива) этого модуля
 * onRecive(uint8_t reciveByte, uint16_t offset) - прием очередного (offset) байта, начатого в onReciveStart
 * onReciveStop(uint16_t paramNum, uint16_t blockSize, int8_t errorCode) - окончание приема блока данных.
 * Если errorCode не 0, то была ошибка в данных. Необходимо проигнорировать принятый с ошибкой блок!
 *
 * int64_t getValue(uint16_t paramNum, int64_t value, int16_t arrayIndex) - запрошено значение параметра.
 * Вызывается после чтения значения параметра из хранилища. Можно изменить отправляемое значение.
 *
 * int64_t setValue(uint16_t paramNum, int64_t value, int16_t arrayIndex=0) - принято значение параметра.
 * Вызывается перед сохранением значения в хранилище (если оно задано). Можно изменить сохраняемое значение.
 *
 * Принцип вызова событий изменения параметров:
 * Отправка данных параметру. Если тип параметра байтовый массив:
 * 1. Вызывается onReciveStart
 * 2. Побайтно для всего блока передаваемых данных, вызывается onRecive
 * 3. После передаси всего блока данных вызывается onReciveStop. Если код ошибки не 0, то надо считать переданные данные сбойными
 * Отправка данных параметру. Если тип параметра не байтовый массив:
 * Вызывается setValue для нового значения параметра.
 * Если значение парамера массив, то для каждого переданного значения массива вызывается setValue
 *
 */

#include <core/net/MWOSNetReciverFields.h>
#include "MWOSParam.h"
#include "MWOSUnit.h"

// типы параметров
#include "MWOSParent.h"
#include "core/adlib/MWBitsMask.h"


/***
 * Интерфейс модуля
 */
class MWOSModuleBase : public MWOSParent {
public:

#pragma pack(push,1)
    uint16_t paramsCount=0; // общее количество параметров модуля
    MWBitsMask changedMask; // битовая маска для признаков изменения значений параметров (размер выделяется автоматом до onInit)
#pragma pack(pop)


    MWOSModuleBase(char * unit_name, uint16_t unit_id=0) : MWOSParent(unit_name,unit_id) {
        unitType=MODULE;
    }

    /**
     * Добавить модулю новый параметр
     * @param param
     */
    bool AddParam(MWOSParam * param) {
        bool res=AddChild(param);
        if (res) paramsCount++;
        return res;
    }

    MWOSParam * getParam(uint16_t paramNum) {
        return (MWOSParam *) MWOSParent::FindChildById(paramNum);
    }

    /***
     * Установить/снять признак необходимости отправки этого параметра на сервер
     * @param param_id  id параметра
     * @param arrayIndex Номер в массиве параметров (если FFFF - все параметры)
     * @param changed  Установить/снять
     */
    void SetParamChangedByParamId(uint16_t param_id, uint16_t arrayIndex, bool changed) {
        uint32_t res=0;
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow!=NULL && paramNow->unitType==UnitType::PARAM) {
            if (paramNow->id==param_id) {
                if (arrayIndex==UINT16_MAX) { // все параметры
                    for (uint16_t i = 0; i < paramNow->arrayCount(); i++) {
                        changedMask.setBit(changed,res+i);
                    }
                } else
                    changedMask.setBit(changed,res+arrayIndex);
                return;
            }
            res+=paramNow->arrayCount();
            paramNow=(MWOSParam *) paramNow->next;
        }
    }

    void SetParamChanged(MWOSParam * param, uint16_t arrayIndex, bool changed) {
        uint32_t res=0;
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow!=NULL && paramNow->unitType==UnitType::PARAM) {
            if (paramNow==param) {
                changedMask.setBit(changed,res+arrayIndex);
                return;
            }
            res+=paramNow->arrayCount();
            paramNow=(MWOSParam *) paramNow->next;
        }
    }

    /**
     * Необходима отправка значения этого параметра на сервер?
     * @param param_id id параметра
     * @return  true - необходима отправка
     */
    bool IsParamChangedByParamId(uint16_t param_id, uint16_t arrayIndex) {
        int32_t res=0;
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow!=NULL && paramNow->unitType==UnitType::PARAM) {
            if (paramNow->id==param_id) {
                return changedMask.getBit(res+arrayIndex);
            }
            res+=paramNow->arrayCount();
            paramNow=(MWOSParam *) paramNow->next;
        }
        return false;
    }

    bool IsParamChanged(MWOSParam * param, uint16_t arrayIndex) {
        int32_t res=0;
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow!=NULL && paramNow->unitType==UnitType::PARAM) {
            if (paramNow==param) {
                return changedMask.getBit(res+arrayIndex);
            }
            res+=paramNow->arrayCount();
            paramNow=(MWOSParam *) paramNow->next;
        }
        return false;
    }

    /***
     * @return Общее количество всех значений всех параметров этого модуля
     */
    int32_t GetParamValuesCount() {
        int32_t res=0;
        MWOSParam * param=(MWOSParam *) child;
        while (param!=NULL && param->unitType==UnitType::PARAM) {
            res+=param->arrayCount();
            param=(MWOSParam *) param->next;
        }
        return res;
    }

    /***
     * Общий битовый размер всех параметров модуля для заданного типа хранилища
     * @param storageType
     * @return
     */
    int32_t bitsSize(int8_t storageType) {
        int32_t bitSize=0;
        MWOSParam * param=(MWOSParam *) child;
        while (param!=NULL && param->unitType==UnitType::PARAM) {
            if (param->storage==storageType) {
                if (param->IsBytes()) { // если это не битовый параметр - выравнивание по байту
                    if ((bitSize & 7) > 0) {
                        int32_t byteSize=bitSize >> 3;
                        byteSize++;
                        bitSize=byteSize << 3;
                    }
                }
                bitSize+=param->bitsSize(true);
            }
            param=(MWOSParam *) param->next;
        }
        return bitSize;
    }

    /***
     * Рассчитать битовое смещение параметра для этого типа хранилища. От начала хранилища для этого модуля.
     * @param paramNum
     * @return
     */
    int32_t getParamBitOffset(MWOSParam * param,int32_t moduleBitOffset) {
        int32_t bitOffset=moduleBitOffset;
        int8_t storageType=param->storage;
        MWOSParam * paramNow=(MWOSParam *) child;
        while (paramNow!=NULL && paramNow->unitType==UnitType::PARAM) {
            if (paramNow->storage==storageType) {
                if (paramNow->IsBytes()) { // если это не битовый параметр - выравнивание по байту
                    if ((bitOffset & 7)>0) {
                        int32_t byteOffset=bitOffset >> 3;
                        byteOffset++;
                        bitOffset=byteOffset << 3;
                    }
                }
                if (param==paramNow) return bitOffset;
                bitOffset+=paramNow->bitsSize(true);
            }
            paramNow=(MWOSParam *) paramNow->next;
        }
        return 0;
    }

    /***
     * Вызывается автоматом после загрузки значений в параметры
     * после включения и перезагрузки вызывается сразу после onFirstInit
     */
    virtual void onInit() {
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {

    }

    /***
     * Получена команда для параметра. Вызывается автоматически при получении новой команды или значения по модулю связи
     * Для блока данных без буфферизации - вызывает сначала команду mwos_server_cmd_param_start_block, потом для каждого байта блока,
     * и в конце mwos_server_cmd_param_stop_block или mwos_server_cmd_param_error_block
     * @param reciverDat   Полученные данные со структурой
     * @return Если команда обработана нормально
     */
    virtual bool onReciveCmd(MWOSProtocolCommand cmd, MWOSNetReciverFields * reciverDat) {
        switch (cmd) {
            case mwos_server_cmd_param_set_value: // не сохранять по месту хранения
                setValueByParamID(reciverDat->reciveValue,reciverDat->param_id,reciverDat->array_index);
                SetParamChangedByParamId(reciverDat->param_id, reciverDat->array_index, true);
                return true;
            case mwos_server_cmd_param_get_value: // запросили значение параметра
                SetParamChangedByParamId(reciverDat->param_id, reciverDat->array_index, true);
                return true;
            case mwos_server_cmd_param_get_param: // запросили данные всего параметра
                SetParamChangedByParamId(reciverDat->param_id, UINT16_MAX, true); // отправить все значения
                return true;
        }
        return false;
    }

    void setValueByParamID(int64_t value, int16_t paramId, int16_t arrayIndex=0) {
        setValue(value, getParam(paramId), arrayIndex);
    }

     int64_t getValueByParamID(int16_t paramId, int16_t arrayIndex=0) {
        return getValue(getParam(paramId), arrayIndex);
    }

    /**
     * Вызывается при запросе значения параметра
     * @param paramNum  Номер параметра
     * @param arrayIndex Номер индекса в массиве значений параметра (если это массив)
     * @return  Значение
     */
    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex=0) {
        return 0;
    }

    /***
     * Вызывается при изменении параметра
     * @param value     Значение для изменения
     * @param param     параметр
     * @param arrayIndex Номер индекса в массиве значений параметра (если это массив)
     */
    virtual void setValue(int64_t value, MWOSParam * param, int16_t arrayIndex=0) {
    }


};


#endif //MWOS3_MWMODULEBASE_H
