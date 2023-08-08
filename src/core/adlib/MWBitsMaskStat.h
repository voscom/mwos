#ifndef MWOS3_MWOSBITSMASK_H
#define MWOS3_MWOSBITSMASK_H
/**
 * Битовая маска
 * @tparam bitsCount Максимальное количество бит
 */

template<uint16_t bitsCount>
class MWBitsMaskStat {
public:
#pragma pack(push,1)
    uint8_t bits[bitsCount/8+ (bitsCount % 8 != 0)];
#pragma pack(pop)

    /**
     * Есть установленные биты
     * @return
     */
    bool haveSetBits() {
        for (uint16_t i = 0; i < sizeof(bits); i++) {
            if (bits[i]>0) return true;
        }
        return false;
    }

    /***
     * Заполнить весь массив одинаковым байтовым значением
     * @param fillValue Байтовое значение (обычно 0 или 0xff)
     */
    void fill(uint8_t fillValue) {
        for (uint16_t i = 0; i < sizeof(bits); i++) {
            bits[i]=fillValue;
        }
    }

    /***
     * Возвращает количество бит в хранилище
     * @return
     */
    size_t length() {
        return bitsCount;
    }

    /**
     * Записать бит
     * @param value
     * @param bit   Номер бита. Если больше bitsCount то задать это значение всем битам
     */
    void setBit(bool value, uint16_t bit) {
        if (bit<bitsCount) {
            uint8_t inBytePos=bit & 7;
            if (value) bitSet(bits[bit >> 3],inBytePos);
            else bitClear(bits[bit >> 3],inBytePos);
        } else {
            for (uint16_t i = 0; i < bitsCount; i++) {
                setBit(value,i);
            }
        }
    }

    /**
     * Прочитать бит
     * @param bit
     * @return
     */
    bool getBit(uint16_t bit) {
        if (bit<bitsCount) return bitRead(bits[bit >> 3],bit);
        return false;
    }

};


#endif //MWOS3_MWOSBITSMASK_H
