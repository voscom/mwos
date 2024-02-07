#ifndef MWOS3_MWBITSMASK_H
#define MWOS3_MWBITSMASK_H
/**
 * Битовая маска
 * Динамически выделяет память
 */

class MWBitsMask {
public:
#pragma pack(push,1)
    int32_t bitsCount=0;
    uint8_t * bits;
#pragma pack(pop)

    void setSize(int32_t bitsSize) {
        bitsCount=bitsSize;
        bits=new uint8_t[bitsCount/8+ (bitsCount % 8 != 0)];
    }

    /***
     * Заполнить весь массив одинаковым байтовым значением
     * @param fillValue Байтовое значение (обычно 0 или 0xff)
     */
    void fill(uint8_t fillValue) {
        for (uint16_t i = 0; i < (bitsCount/8+ (bitsCount % 8 != 0)); i++) {
            bits[i]=fillValue;
        }
    }

    size_t bytesSize() {
        return bitsCount/8+ (bitsCount % 8 != 0);
    }

    /**
     * Есть установленные биты
     * @return
     */
    bool haveSetBits() {
        for (int32_t i = 0; i < bytesSize(); i++) {
            if (bits[i]>0) return true;
        }
        return false;
    }

    /***
     * Возвращает количество бит в хранилище
     * @return
     */
    size_t length() {
        return bitsCount;
    }

    /***
     * Вернуть номер первого установленного бита
     * @return
     */
    int32_t firstSetBit() {
        for (int32_t i = 0; i < (bitsCount/8+ (bitsCount % 8 != 0)); i++) {
            if (bits[i]>0) {
                for (uint8_t j = 0; j < 8; j++) {
                    if (bitRead(bits[i],j)) return (i << 3)+j;
                }
            }
        }
        return -1;
    }

    /**
     * Записать бит
     * @param value
     * @param bit   Номер бита
     */
    void setBit(bool value, int32_t bit) {
        if (bit<bitsCount) {
            uint8_t inBytePos=bit & 7;
            if (value) bitSet(bits[bit >> 3],inBytePos);
            else bitClear(bits[bit >> 3],inBytePos);
        }
    }

    /**
     * Прочитать бит
     * @param bit
     * @return
     */
    bool getBit(int32_t bit) {
        bool value=false;
        uint8_t inBytePos=bit & 7;
        if (bit<bitsCount) value=bitRead(bits[bit >> 3],inBytePos);
        return value;
    }

};


#endif //MWOS3_MWBITSMASK_H
