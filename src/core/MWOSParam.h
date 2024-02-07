#ifndef MWOS3_MWOSPARAM_H
#define MWOS3_MWOSPARAM_H
/***
 * Параметр модуля
 * Для получения данных параметра, необходимо использовать соответствующие методы модуля
 *
 */
#include "MWOSConsts.h"
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
} p_##_p_name;
//VALUE_##_p_value_type _##_p_name[_p_array_length]={ AddChild(&p_##_p_name) };
#define MWOS_PARAM_F(_p_id, _p_name, _p_value_type, _p_param_group, _p_storage, _p_array_length, _p_format) \
class CLASS_MWOS_PARAM_##_p_name: public MWOSParam { \
    public: \
    CLASS_MWOS_PARAM_##_p_name() : MWOSParam(_p_id, (char *) F(#_p_format), _p_value_type, ((MWOSParamGroup) ( _p_param_group )), _p_storage, _p_array_length) { } \
} p_##_p_name;


class MWOSParam : public MWOSUnit {
public:

#pragma pack(push,1)
    /**
     * тип данных заданный константой ParamValueType
     * может содержать однотипный фиксированный массив, тогда задается так:
     * {mwos_param_array - признак массива} * {кол-во элементов массива} + {mwos_param_int16 - тип данных в массиве}
     * Или размер блока байтовых данных параметра 0..32k (задается просто положительным числом >0)
     */
    MWOS_PARAM_INDEX_UINT arrayLength; // 8bit или 16bit
    ParamValueType valueType; // 8bit
    MWOSParamGroup group; // 8bit - к какой группе относится параметр
    MWOSParamStorage storage; // 8-bit - Тип хранилища
#pragma pack(pop)

    MWOSParam(int32_t param_id, char * param_name, ParamValueType value_type, MWOSParamGroup param_group, MWOSParamStorage param_storage, MWOS_PARAM_INDEX_UINT param_array_length=1) : MWOSUnit(param_name,param_id) {
        group=param_group;
        storage=param_storage;
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
        if (storageType<0) return false;
        return storage==storageType;
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
     * Вызывать для этого параметра методы onReciveStart onRecive onReciveStop или false = метод setValue
     * @return
     */
    bool IsLong() {
        return (valueType<=mwos_param_string || valueType>=mwos_param_byte_array);
    }

    /**
     * Необходимо вырывнивать этот параметр по байту?
     * @return
     */
    bool IsBytes() {
        uint8_t onlyType=(valueType & 31);
        return onlyType < mwos_param_bits1 || onlyType > mwos_param_bits7;
    }

    /***
     * Возвращает количество однотипных данных в этом параметре
     * @return Если больше 1 - это массив
     */
    MWOS_PARAM_INDEX_UINT arrayCount() {
        return arrayLength;
    }

     /**
      * Возвращает размер типовых данных параметра в битах
      * @param total   Суммарный обьем всех элементов массива (если данные параметра - массив), false - только одного элемента
      * @return
      */
    uint32_t bitsSize(bool total) {
        uint32_t res=0;
        switch (valueType & 31) {
            case mwos_param_int8: res=8; break;
            case mwos_param_uint8: res=8; break;
            case mwos_param_int16: res=16; break;
            case mwos_param_uint16: res=16; break;
            case mwos_param_int32: res=32; break;
            case mwos_param_uint32: res=32; break;
            case mwos_param_int64: res=64; break;
            case mwos_param_uint64: res=64; break;
            case mwos_param_float32: res=32; break;
            case mwos_param_double64: res=64; break;
            case mwos_param_bits1: res=1; break;
            case mwos_param_bits2: res=2; break;
            case mwos_param_bits3: res=3; break;
            case mwos_param_bits4: res=4; break;
            case mwos_param_bits5: res=5; break;
            case mwos_param_bits6: res=6; break;
            case mwos_param_bits7: res=7; break;

            default: res=8; // для остальных типов - длина 8 bit
        }
        if (total) res *= arrayCount();
        return res;
    }


};


#endif //MWOS3_MWOSPARAM_H
