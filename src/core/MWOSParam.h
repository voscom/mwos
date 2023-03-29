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
    CLASS_MWOS_PARAM_##_p_name() : MWOSParam(_p_id, (char *) F(#_p_name), _p_value_type, _p_param_group, _p_storage, _p_array_length) { } \
} p_##_p_name;
//VALUE_##_p_value_type _##_p_name[_p_array_length]={ AddChild(&p_##_p_name) };


class MWOSParam : public MWOSUnit {
public:

#pragma pack(push,1)
    /**
     * тип данных заданный константой ParamValueType
     * может содержать однотипный фиксированный массив, тогда задается так:
     * {mwos_param_array - признак массива} * {кол-во элементов массива} + {mwos_param_int16 - тип данных в массиве}
     * Или размер блока байтовых данных параметра 0..32k (задается просто положительным числом >0)
     */
    int16_t valueType; // 16bit
    MWOSParamGroup group; // 8bit - к какой группе относится параметр
    MWOSParamStorage storage; // 8-bit - Тип хранилища
#pragma pack(pop)

    MWOSParam(int32_t param_id, char * param_name, ParamValueType value_type, MWOSParamGroup param_group, MWOSParamStorage param_storage, uint16_t param_array_length=0) : MWOSUnit(param_name,param_id) {
        group=param_group;
        storage=param_storage;
        valueType=value_type+(int16_t)mwos_param_array*param_array_length;
        if (param_array_length<1) param_array_length=1;
    }

    bool IsReadOnly() {
        return (group & mwos_param_readonly) > 0;
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
     * Необходимо сохранять изменения в журнал
     * @return
     */
    bool IsLogEvent() {
        return (group & mwos_param_event) > 0;
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
        return (valueType<=mwos_param_string);
    }

    /**
     * Необходимо вырывнивать этот параметр по байту?
     * @return
     */
    bool IsBytes() {
        if (valueType<0) return true;
        return (valueType & 31) < mwos_param_bits1;
    }

    /***
     * Возвращает количество однотипных данных в этом параметре
     * @return Если больше 1 - это массив
     */
    uint16_t arrayCount() {
        if (valueType<1) return 1;
        int16_t arrayCount=valueType >> 5;
        if (arrayCount<1) arrayCount=1;
        return arrayCount;
    }

     /**
      * Возвращает размер типовых данных параметра в битах
      * @param total   Суммарный обьем всех элементов массива (если данные параметра - массив), false - только одного элемента
      * @return
      */
    int32_t bitsSize(bool total) {
        if (valueType<0) return abs(valueType) << 3;
        int32_t res=0;
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
        }
        if (total) res *= arrayCount();
        return res;
    }


};


#endif //MWOS3_MWOSPARAM_H
