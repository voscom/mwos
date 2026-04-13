#ifndef MWVALUE_H
#define MWVALUE_H

#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <new>
#include "core/MWOSConsts.h"

#ifndef DecimalPlaceDefault
// количество знаков после запятой для преобразования дробей в строку
#define  DecimalPlaceDefault 2
#endif

// ========== Класс MWValue ==========
class MWValue {
private:
    uint8_t* bytes = nullptr;

    // Вспомогательная: сравнение буферов
    bool compareBytes(const uint8_t* other, size_t len) const {
        if (!bytes && !other) return true;
        if (!bytes || !other) return false;
        return memcmp(bytes, other, len) == 0;
    }

public:
    uint16_t module_id = 0xffff;
    uint16_t param_id = 0xffff;
    uint16_t param_index = 0xffff;
    uint16_t size=0;
    ParamValueType type = PARAM_AUTO;
    union ValueStatus {
        uint8_t raw;
        struct {
            uint8_t autoMemory : 1; // признак, что необходимо автоматически управлять памятью
            uint8_t error : 1; // признак ошибки
            uint8_t loaded : 1; // данные были успешно загружены
            uint8_t : 5;
        };
        ValueStatus() : raw(1) {}
    };
    ValueStatus status;

    // ========== Конструкторы и деструктор ==========

    MWValue() = default;

    // Конструктор копирования (глубокое копирование)
    MWValue(const MWValue& other)
        : module_id(other.module_id), param_id(other.param_id),
          param_index(other.param_index), type(other.type), bytes(nullptr) {
        if (other.bytes) {
            size = other.size;
            if (size > 0) {
                bytes = new (std::nothrow) uint8_t[size];
                if (bytes) {
                    memcpy(bytes, other.bytes, size);
                }
            }
        }
    }

    // Конструкторы для прямого присваивания значений
    MWValue(ParamValueType t, uint16_t moduleId, uint16_t paramId, uint16_t index ): MWValue() { type=t; module_id=moduleId; param_id=paramId; param_index=index; }
    MWValue(int8_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(int16_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(int32_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(int64_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(uint8_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(uint16_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(uint32_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(uint64_t v, ParamValueType t)  : MWValue() { setValueInt(v); }
    MWValue(double v, ParamValueType t)   : MWValue() { setValueDouble(v); }
    MWValue(float v, ParamValueType t)   : MWValue() { setValueDouble(v); }
    MWValue(const char* str, ParamValueType t) : MWValue() { setString(str); }

    void init(ParamValueType t, uint16_t moduleId, uint16_t paramId, uint16_t index) { type=t; module_id=moduleId; param_id=paramId; param_index=index; }

    // Деструктор (всегда освобождает память)
    ~MWValue() {
        stop();
    }

    // Оператор присваивания (глубокое копирование)
    MWValue& operator=(const MWValue& other) {
        if (this != &other) {
            // Освобождаем старую память
            stop();
            // Копируем метаданные
            module_id = other.module_id;
            param_id = other.param_id;
            param_index = other.param_index;
            type = other.type;
            // Копируем данные
            if (other.bytes) {
                size = other.size;
                if (size > 0) {
                    bytes = new (std::nothrow) uint8_t[size];
                    if (bytes) {
                        memcpy(bytes, other.bytes, size);
                    }
                }
            } else {
                bytes = nullptr;
            }
        }
        return *this;
    }

    // ========== Управление памятью ==========

    void start(size_t newSize, bool clear=false) {
        if (!newSize) return;
        if (bytes && newSize==size) return;
        if (status.autoMemory) {
            stop();
            bytes = new (std::nothrow) uint8_t[newSize];
        }
        if (status.autoMemory || size > newSize) size=newSize;
        if (clear && bytes) memset(bytes,0,size); // заполним нулями
    }

    void stop() {
        if (bytes) {
            if (status.autoMemory) delete[] bytes;
            bytes = nullptr;
            type = PARAM_AUTO;
            status.autoMemory=true;
            status.error=false;
            status.loaded=false;
            size=0;
        }
    }

    // ========== Сеттеры ==========

    void setInt8(int8_t v) { start(1); if (bytes) { bytes[0] = static_cast<uint8_t>(v); type = PARAM_INT8; } }

    void setUInt8(uint8_t v) {
        start(1);
        if (bytes) { bytes[0] = v; type = PARAM_UINT8; }
    }

    void setInt16(int16_t v) {
        start(2);
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_INT16; }
    }

    void setUInt16(uint16_t v) {
        start(2);
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_UINT16; }
    }

    void setInt32(int32_t v) {
        start(4);
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_INT32; }
    }

    void setUInt32(uint32_t v) {
        start(4);
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_UINT32; }
    }

    void setInt64(int64_t v) {
        start(8);
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_INT64; }
    }

    void setUInt64(uint64_t v) {
        start(8);
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_UINT64; }
    }

    void setFloat(float v) {
        start(sizeof(v));
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_FLOAT32; }
    }

    void setDouble(double v) {
        start(sizeof(v));
        if (bytes) { memcpy(bytes, &v, sizeof(v)); type = PARAM_DOUBLE64; }
    }

    /**
     * Установить указатель данных на готовый буфер и отключает автоменеджмент памяти.
     * Что-бы сразу копировали в этот буфер.
     * @param buff  Ранее выделенный буфер для этого значения.
     * @param maxLen   Максимальный размер этого значения.
     * @param autoMemory   Автоматически освобождать память
     */
    void setBuffer(uint8_t* buff, size_t maxLen, bool autoMemory=false) {
        stop();
        bytes=buff;
        size=maxLen;
        status.autoMemory=autoMemory;
        status.loaded=false;
        status.error=false;
    }

    size_t setString(const char* str) {
        if (!str) return 0;
        size_t len = strlen(str);
        if (len>1 && str[0]=='"' && str[len-1]=='"') { // избавимся от кавычек
            str++;
            len-=2;
        }
        if (len<1 || str[len-1]!=0) len++; // нужен концевик
        start(len);
        if (bytes) {
            if (len>1) memcpy(bytes, str, len-1);
            bytes[len-1] = 0;
            type = PARAM_STRING;
            size = len;
            return size;
        }
        type = PARAM_AUTO;
        return 0;
    }

    // Битовые поля
    template<uint8_t Bits>
    void setBits(uint8_t v) {
        static_assert(Bits >= 1 && Bits <= 7, "Bits must be 1-7");
        start(1);
        if (bytes) {
            uint32_t mask = (1U << Bits) - 1;
            bytes[0] = static_cast<uint8_t>(v & mask);
            type = static_cast<ParamValueType>(PARAM_BITS1 + Bits - 1);
        }
    }

    void setBits1(uint8_t v) { setBits<1>(v); }
    void setBits2(uint8_t v) { setBits<2>(v); }
    void setBits3(uint8_t v) { setBits<3>(v); }
    void setBits4(uint8_t v) { setBits<4>(v); }
    void setBits5(uint8_t v) { setBits<5>(v); }
    void setBits6(uint8_t v) { setBits<6>(v); }
    void setBits7(uint8_t v) { setBits<7>(v); }

    // ========== Геттеры ==========

    int8_t getInt8() const      { return bytes ? static_cast<int8_t>(bytes[0]) : 0; }
    uint8_t getUInt8() const    { return bytes ? bytes[0] : 0; }
    int16_t getInt16() const    { int16_t v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    uint16_t getUInt16() const  { uint16_t v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    int32_t getInt32() const    { int32_t v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    uint32_t getUInt32() const  { uint32_t v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    int64_t getInt64() const    { int64_t v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    uint64_t getUInt64() const  { uint64_t v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    float getFloat() const      { float v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    double getDouble() const    { double v = 0; if(bytes) memcpy(&v, bytes, sizeof(v)); return v; }
    uint8_t getBits() const     { return bytes ? bytes[0] : 0; }

    ParamValueType getType() const { return type; }
    uint8_t* getBytes() const { return bytes; }

    bool isNumeric() const { return type >= PARAM_INT8 && type <= PARAM_DOUBLE64; }
    bool isInt() const { return (type >= PARAM_INT8 && type <= PARAM_UINT64) || (type >= PARAM_BITS1 && type <= PARAM_BITS7); }
    bool isFloat() const { return type == PARAM_FLOAT32 || type == PARAM_DOUBLE64; }
    bool isString() const { return type == PARAM_STRING; }
    bool isBits() const { return type >= PARAM_BITS1 && type <= PARAM_BITS7; }
    bool isAuto() const { return type == PARAM_AUTO; }
    bool isEmpty() const { return !bytes; }

    // ========== Вспомогательная: размер данных ==========

    // ========== toString ==========

    String toString(unsigned char delimiter = ' ', unsigned int decimalPlaces = DecimalPlaceDefault, unsigned char base = 10) const {
        if (!bytes) return String();
        if (isFloat()) return String(toDouble(), decimalPlaces);
        if (isInt()) return String(toInt(), base);
        if (isString()) return String((const char*)bytes);
        return "";
    }

    String toInfo() const {
        return  String(F("MWValue("))+String(type)+';'+String(size)+')'+String(module_id)+':'+String(param_id)+':'+String(param_index)+'='+toString() ;
    }

    // ========== parseString ==========

    int parseString(const char* str) {
        if (!str || !bytes || type == PARAM_AUTO) return -1;
        while (*str == ' ' || *str == '\t') str++;
        if (!*str) return 0;

        int base = 10;
        if (str[0] == '0') {
            if (str[1] == 'x' || str[1] == 'X') { base = 16; str += 2; }
            else if (str[1] == 'b' || str[1] == 'B') { base = 2; str += 2; }
            else if (str[1] >= '0' && str[1] <= '7') { base = 8; }
        }

        char* endptr;

        switch (type) {
            case PARAM_STRING:
                return setString(str) > 0 ? 0 : -1;

            case PARAM_INT8: {
                long v = strtol(str, &endptr, base);
                if (endptr == str || v < INT8_MIN || v > INT8_MAX) return (int)(endptr - str);
                setInt8((int8_t)v);
                return 0;
            }
            case PARAM_UINT8: {
                unsigned long v = strtoul(str, &endptr, base);
                if (endptr == str || v > UINT8_MAX) return (int)(endptr - str);
                setUInt8((uint8_t)v);
                return 0;
            }
            case PARAM_INT16: {
                long v = strtol(str, &endptr, base);
                if (endptr == str || v < INT16_MIN || v > INT16_MAX) return (int)(endptr - str);
                setInt16((int16_t)v);
                return 0;
            }
            case PARAM_UINT16: {
                unsigned long v = strtoul(str, &endptr, base);
                if (endptr == str || v > UINT16_MAX) return (int)(endptr - str);
                setUInt16((uint16_t)v);
                return 0;
            }
            case PARAM_INT32: {
                long v = strtol(str, &endptr, base);
                if (endptr == str) return (int)(endptr - str);
                setInt32(v);
                return 0;
            }
            case PARAM_UINT32: {
                unsigned long v = strtoul(str, &endptr, base);
                if (endptr == str) return (int)(endptr - str);
                setUInt32(v);
                return 0;
            }
            case PARAM_INT64: {
                long long v = strtoll(str, &endptr, base);
                if (endptr == str) return (int)(endptr - str);
                setInt64(v);
                return 0;
            }
            case PARAM_UINT64: {
                unsigned long long v = strtoull(str, &endptr, base);
                if (endptr == str) return (int)(endptr - str);
                setUInt64(v);
                return 0;
            }
            case PARAM_FLOAT32: {
                float v = strtof(str, &endptr);
                if (endptr == str) return (int)(endptr - str);
                setFloat(v);
                return 0;
            }
            case PARAM_DOUBLE64: {
                double v = strtod(str, &endptr);
                if (endptr == str) return (int)(endptr - str);
                setDouble(v);
                return 0;
            }
            case PARAM_BITS1: case PARAM_BITS2: case PARAM_BITS3:
            case PARAM_BITS4: case PARAM_BITS5: case PARAM_BITS6: case PARAM_BITS7: {
                uint8_t bits = static_cast<uint8_t>(type) - PARAM_BITS1 + 1;
                unsigned long v = strtoul(str, &endptr, base);
                uint32_t mask = (1U << bits) - 1;
                if (endptr == str || (v & ~mask) != 0) return (int)(endptr - str);
                start(1);
                if (bytes) { bytes[0] = static_cast<uint8_t>(v & mask); }
                return 0;
            }
            default:
                return -1;
        }
    }

    // ========== Операторы сравнения ==========

    bool operator==(const MWValue& other) const {
        if (type != other.type) return false;
        if (module_id != other.module_id) return false;
        if (param_id != other.param_id) return false;
        if (param_index != other.param_index) return false;
        if (size == 0) return true;
        return compareBytes(other.bytes, size);
    }

    bool operator!=(const MWValue& other) const {
        return !(*this == other);
    }

    bool valueEquals(const MWValue& other) const {
        if (type != other.type) return false;
        if (size == 0) return true;
        return compareBytes(other.bytes, size);
    }

    bool operator==(int32_t v) const {
        return type == PARAM_INT32 && getInt32() == v;
    }
    bool operator==(uint32_t v) const {
        return type == PARAM_UINT32 && getUInt32() == v;
    }
    bool operator==(float v) const {
        return type == PARAM_FLOAT32 && getFloat() == v;
    }
    bool operator==(const char* str) const {
        return type == PARAM_STRING && bytes && strcmp((const char*)bytes, str) == 0;
    }

    // ========== Операторы преобразования типа (C++11 explicit) ==========

    explicit operator int8_t() const    { return (int8_t)toInt(); }
    explicit operator uint8_t() const   { return (uint8_t)toInt(); }
    explicit operator int16_t() const   { return (int16_t)toInt(); }
    explicit operator uint16_t() const  { return (uint16_t)toInt(); }
    explicit operator int32_t() const   { return (int32_t)toInt(); }
    explicit operator uint32_t() const  { return (uint32_t)toInt(); }
    explicit operator int64_t() const   { return toInt(); }
    explicit operator uint64_t() const  { return (uint64_t)toInt(); }
    explicit operator float() const     { return (float)toDouble(); }
    explicit operator double() const    { return toDouble(); }
    explicit operator const uint8_t*() const { return bytes; }
    explicit operator const char*() const { return bytes ? (const char*)bytes : ""; }
    explicit operator bool() const {
        if (type >= PARAM_BITS1 && type <= PARAM_BITS7) {
            return getBits() != 0;
        }
        return isNumeric() ? (toInt() != 0) : (bytes != nullptr);
    }
    explicit operator String() const { return toString(); }

    // ========== Именованные методы ==========

    uint8_t* toBytes() { return bytes; }
    const uint8_t* toBytes() const { return bytes; }
    const char* toStr() const { return bytes ? (const char*)bytes : ""; }

    // ========== Универсальные конвертеры ==========
    int64_t getValue(int64_t def = 0) { return toInt(def); }
    int32_t getValue(int32_t def = 0) { return toInt(def); }
    int16_t getValue(int16_t def = 0) { return toInt(def); }
    int8_t getValue(int8_t def = 0) { return toInt(def); }
    uint64_t getValue(uint64_t def = 0) { return toInt(def); }
    uint32_t getValue(uint32_t def = 0) { return toInt(def); }
    uint16_t getValue(uint16_t def = 0) { return toInt(def); }
    uint8_t getValue(uint8_t def = 0) { return toInt(def); }

    int64_t toInt(int64_t def = 0) const {
        if (!bytes) return def;
        switch (type) {
            case PARAM_INT8:    return (int64_t)getInt8();
            case PARAM_UINT8:   return (int64_t)getUInt8();
            case PARAM_INT16:   return (int64_t)getInt16();
            case PARAM_UINT16:  return (int64_t)getUInt16();
            case PARAM_INT32:   return (int64_t)getInt32();
            case PARAM_UINT32:  return (int64_t)getUInt32();
            case PARAM_INT64:   return getInt64();
            case PARAM_UINT64:  return (int64_t)getUInt64();
            case PARAM_BITS1: case PARAM_BITS2: case PARAM_BITS3:
            case PARAM_BITS4: case PARAM_BITS5: case PARAM_BITS6:
            case PARAM_BITS7:   return (int64_t)getBits();
            case PARAM_FLOAT32: return (int64_t)getFloat();
            case PARAM_DOUBLE64: return (int64_t)getDouble();
            case PARAM_STRING: {
                const char* str = (const char*)bytes;
                char* endptr;
                long long v = strtoll(str, &endptr, 0);
                return (endptr != str) ? v : def;
            }
            default:
                return def;
        }
    }

    double getValue(double def = 0) { return toDouble(def); }
    float getValue(float def = 0) { return toDouble(def); }
    String getValue() { return toString(); }

    double toDouble(double def = 0.0) const {
        if (!bytes) return def;
        switch (type) {
            case PARAM_INT8:    return (double)getInt8();
            case PARAM_UINT8:   return (double)getUInt8();
            case PARAM_INT16:   return (double)getInt16();
            case PARAM_UINT16:  return (double)getInt16();
            case PARAM_INT32:   return (double)getInt32();
            case PARAM_UINT32:  return (double)getUInt32();
            case PARAM_INT64:   return (double)getInt64();
            case PARAM_UINT64:  return (double)getUInt64();
            case PARAM_BITS1: case PARAM_BITS2: case PARAM_BITS3:
            case PARAM_BITS4: case PARAM_BITS5: case PARAM_BITS6:
            case PARAM_BITS7:   return (double)getBits();
            case PARAM_FLOAT32: return (double)getFloat();
            case PARAM_DOUBLE64: return getDouble();
            case PARAM_STRING: {
                const char* str = (const char*)bytes;
                char* endptr;
                double v = strtod(str, &endptr);
                return (endptr != str) ? v : def;
            }
            default:
                return def;
        }
    }

    // ========== Универсальные сеттеры с конвертацией ==========

    void setValue(const String &v) { setString(v.c_str()); }
    void setValue(const char * v) { setString(v); }
    void setValue(char * v) { setString(v); }
    void setValue(double v, ParamValueType newType=PARAM_AUTO) { setValueDouble(v,newType); }
    void setValue(float v, ParamValueType newType=PARAM_AUTO) { setValueDouble(v,newType); }

    // Установить значение из double, конвертируя в текущий type
    void setValueDouble(double v, ParamValueType newType=PARAM_AUTO) {
        if (newType!=PARAM_AUTO) type=newType;
        if (type==PARAM_AUTO) type=PARAM_DOUBLE64;
        switch (type) {
            // Целочисленные типы
            case PARAM_INT8:    { setInt8(static_cast<int8_t>(v)); } break;
            case PARAM_UINT8:   { setUInt8(static_cast<uint8_t>(v)); } break;
            case PARAM_INT16:   { setInt16(static_cast<int16_t>(v)); } break;
            case PARAM_UINT16:  { setUInt16(static_cast<uint16_t>(v)); } break;
            case PARAM_INT32:   { setInt32(static_cast<int32_t>(v)); } break;
            case PARAM_UINT32:  { setUInt32(static_cast<uint32_t>(v)); } break;
            case PARAM_INT64:   { setInt64(static_cast<int64_t>(v)); } break;
            case PARAM_UINT64:  { setUInt64(static_cast<uint64_t>(v)); } break;
                // Плавающая точка
            case PARAM_FLOAT32: { setFloat(static_cast<float>(v)); } break;
            case PARAM_DOUBLE64: { setDouble(v); } break;
                // Битовые поля
            case PARAM_BITS1: setBits<1>(static_cast<int8_t>(v)); break;
            case PARAM_BITS2: setBits<2>(static_cast<int8_t>(v)); break;
            case PARAM_BITS3: setBits<3>(static_cast<int8_t>(v)); break;
            case PARAM_BITS4: setBits<4>(static_cast<int8_t>(v)); break;
            case PARAM_BITS5: setBits<5>(static_cast<int8_t>(v)); break;
            case PARAM_BITS6: setBits<6>(static_cast<int8_t>(v)); break;
            case PARAM_BITS7: setBits<7>(static_cast<int8_t>(v)); break;
                // Строка
            case PARAM_STRING: { setString(String(v,DecimalPlaceDefault).c_str()); } break;
            default:
                break;  // Ничего не делаем для неподдерживаемых типов
        }
    }

    void setValue(int64_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(int32_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(int16_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(int8_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(uint64_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(uint32_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(uint16_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }
    void setValue(uint8_t v, ParamValueType newType=PARAM_AUTO) { setValueInt(v,newType); }

    // Установить значение из int64_t, конвертируя в текущий type
    void setValueInt(int64_t v, ParamValueType newType=PARAM_AUTO) {
        if (newType!=PARAM_AUTO) type=newType;
        if (type==PARAM_AUTO) type=PARAM_INT64;
        switch (type) {
            // Целочисленные типы
            case PARAM_INT8:    { setInt8(static_cast<int8_t>(v)); } break;
            case PARAM_UINT8:   { setUInt8(static_cast<uint8_t>(v)); } break;
            case PARAM_INT16:   { setInt16(static_cast<int16_t>(v)); } break;
            case PARAM_UINT16:  { setUInt16(static_cast<uint16_t>(v)); } break;
            case PARAM_INT32:   { setInt32(static_cast<int32_t>(v)); } break;
            case PARAM_UINT32:  { setUInt32(static_cast<uint32_t>(v)); } break;
            case PARAM_INT64:   { setInt64(v); } break;
            case PARAM_UINT64:  { setUInt64(static_cast<uint64_t>(v)); } break;
                // Плавающая точка (целое в дробное)
            case PARAM_FLOAT32: { setFloat(static_cast<float>(v)); } break;
            case PARAM_DOUBLE64: { setDouble(static_cast<double>(v)); } break;
                // Битовые поля
            case PARAM_BITS1: setBits<1>(static_cast<int8_t>(v)); break;
            case PARAM_BITS2: setBits<2>(static_cast<int8_t>(v)); break;
            case PARAM_BITS3: setBits<3>(static_cast<int8_t>(v)); break;
            case PARAM_BITS4: setBits<4>(static_cast<int8_t>(v)); break;
            case PARAM_BITS5: setBits<5>(static_cast<int8_t>(v)); break;
            case PARAM_BITS6: setBits<6>(static_cast<int8_t>(v)); break;
            case PARAM_BITS7: setBits<7>(static_cast<int8_t>(v)); break;
                // Строка
            case PARAM_STRING: { setString(String(v).c_str()); } break;
            default:
                break;
        }
    }

}; // что-бы не выравнивало

#endif