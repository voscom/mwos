#ifndef MWOS3_MWOSPARAM_H
#define MWOS3_MWOSPARAM_H
/***
 * Параметр модуля
 * Для получения данных параметра, необходимо использовать соответствующие методы модуля
 *
 */
#include "MWOSConsts.h"
#include "MWOSDebug.h"
#include "MWOSUnit.h"

/**
 * Макрос определяет параметр. Создает дочерний класс с переопределением методов и свойств
 * Так же создает два поля:
 * p_имя - поле для прямого доступа к параметру
 * _имя - переменная для прямого доступа к значению (только чтение)
 */

#define MWOS_PARAM(_p_id, _p_name, _p_value_type, _p_param_group, _p_storage, _p_array_length) \
class CLASS_MWOS_PARAM_##_p_name: public MWOSParam { \
    public: \
    CLASS_MWOS_PARAM_##_p_name() : MWOSParam(_p_id, (char *) F(#_p_name), _p_value_type, ((MWOSParamGroup) ( _p_param_group )), _p_storage, _p_array_length) { } \
} p_##_p_name; \
enum { id_##_p_name = _p_id }; \
struct REG_##_p_name { \
REG_##_p_name(MWOSModule* module, MWOSParam& param) { module->AddParam(param); } \
} r_##_p_name { this, p_##_p_name };

#define MWOS_PARAM_FF(_p_id, _p_name, _p_value_type, _p_param_group, _p_storage, _p_array_length, _p_format) \
class CLASS_MWOS_PARAM_##_p_name: public MWOSParam { \
public: \
CLASS_MWOS_PARAM_##_p_name() : MWOSParam(_p_id, (char *) F("{'name':'" #_p_name "','value_format':'" _p_format "'}"), _p_value_type, ((MWOSParamGroup) ( _p_param_group )), _p_storage, _p_array_length) { } \
} p_##_p_name; \
enum { id_##_p_name = _p_id }; \
struct REG_##_p_name { \
REG_##_p_name(MWOSModule* module, MWOSParam& param) { module->AddParam(param); } \
} r_##_p_name { this, p_##_p_name };


class MWOSParam : public MWOSUnit {
public:
#pragma pack(push,1)
    /**
     * тип данных заданный константой ParamValueType
     * может содержать однотипный фиксированный массив, тогда задается так:
     * {mwos_param_array - признак массива} * {кол-во элементов массива} + {mwos_param_int16 - тип данных в массиве}
     * Или размер блока байтовых данных параметра 0..32k (задается просто положительным числом >0)
     */
    int16_t arrayLength; // 16bit
    ParamValueType valueType; // 8bit
    MWOSParamGroup group; // 8bit - к какой группе относится параметр
#pragma pack(pop)

    MWOSParam(int32_t param_id, char * param_name, ParamValueType value_type, MWOSParamGroup param_group, uint8_t param_storage, MWOS_PARAM_INDEX_UINT param_array_length=1) : MWOSUnit(param_name,param_id) {
        group=param_group;
        storage=(param_storage & 7); // Тип хранилища
        saveToLog=(param_storage>7); // параметр сохранять в журнал (может быть перенастроено удаленно). Только, если задан модуль журнала.
        arrayLength=param_array_length;
        if (arrayLength==0) arrayLength=1;
        valueType=value_type;
        if (param_array_length<1) param_array_length=1;
    }

    /**
     * Это хранилище заданного типа?
     * @param storageType
     * @return
     */
    bool IsStorage(int8_t storageType) {
        if (storageType<0 || storageType >= MWOS_STORAGE_NO) return false;
        return storage==storageType;
    }

    /**
     * Вернуть хранилище параметра
     * @return
     */
    uint8_t getStorage() {
        return storage;
    }

    /**
     * Этот параметр относится к заданной группе?
     * @param paramGroup
     * @return
     */
    bool IsGroup(MWOSParamGroup paramGroup) {
        if (paramGroup<32) return (group & 31)==paramGroup; // это тип параметра
        return (group & paramGroup)>0; // это группа параметра
    }

    /**
     * Это параметр управления?
     * @return
     */
    bool IsParamControl() {
        uint8_t groupNum=(group & 31);
        return ((group & (PARAM_TYPE_SECRET | PARAM_TYPE_OPTIONS))==0) && groupNum!=PARAM_TYPE_PIN && groupNum!=PARAM_TYPE_FILE;
    }

    /**
     * Возвращает размер данных параметра в байтах.
     * Битовый остаток каждого значения выравнивается вверх до байта.
     * @return
     */
    uint16_t byteSize(bool total) {
        int32_t bitSize=bitsSize(false);
        uint16_t res=bitSize>>3;
        if ((bitSize & 7)>0) res++;
        if (total) res*=arrayCount();
        return res;
    }

    /**
     * Необходимо вырывнивать этот параметр по байту?
     * @return
     */
    bool IsBit() {
        return valueType >= PARAM_BITS1 && valueType <= PARAM_BITS7;
    }

    bool IsFloat() {
        return (valueType==PARAM_FLOAT32 || valueType==PARAM_DOUBLE64);
    }

    bool IsInt() {
        return (valueType>=PARAM_INT8 && valueType<=PARAM_FLOAT32) || (valueType>=PARAM_BITS1 && valueType<=PARAM_BITS7);
    }

    bool IsString() {
        return (valueType==PARAM_STRING);
    }

    /***
     * Возвращает общий размер массива в байтах
     * @return Если больше 1 - это массив
     */
    MWOS_PARAM_INDEX_UINT arrayCount() {
        return arrayLength;
    }

    /***
     * Возвращает количество однотипных данных в этом параметре
     * @return Если больше 1 - это массив (для строк всегда 1)
     */
    MWOS_PARAM_INDEX_UINT indexCount() {
        if (IsString()) return 1;
        return arrayLength;
    }

    /**
     * Размер строки из буфера для этого параметра (заканчивается на 0)
     * @param buff  буфер со строкой, заканчивающейся на 0
     * @return  Размер строки
     */
    uint16_t stringSize(uint8_t * buff) {
        uint16_t len=byteSize(true);
        if (!IsString()) return len;
        for (uint16_t i = 0; i < len; i++) {
            if (buff[i]==0) return i;
        }
        return len;
    }

     /**
      * Возвращает размер типовых данных параметра в битах
      * @param total   Суммарный обьем всех элементов массива (если данные параметра - массив), false - только одного элемента
      * @return
      */
    uint32_t bitsSize(bool total) {
        uint32_t res=0;
        switch (valueType) {
            case PARAM_INT8: res=8; break;
            case PARAM_UINT8: res=8; break;
            case PARAM_INT16: res=16; break;
            case PARAM_UINT16: res=16; break;
            case PARAM_INT32: res=32; break;
            case PARAM_UINT32: res=32; break;
            case PARAM_INT64: res=64; break;
            case PARAM_UINT64: res=64; break;
            case PARAM_FLOAT32: res=32; break;
            case PARAM_DOUBLE64: res=64; break;
            case PARAM_BITS1: res=1; break;
            case PARAM_BITS2: res=2; break;
            case PARAM_BITS3: res=3; break;
            case PARAM_BITS4: res=4; break;
            case PARAM_BITS5: res=5; break;
            case PARAM_BITS6: res=6; break;
            case PARAM_BITS7: res=7; break;
            default: res=8; // для остальных типов - длина 8 bit
        }
        if (total) res *= arrayCount();
        return res;
    }

};


#endif //MWOS3_MWOSPARAM_H
