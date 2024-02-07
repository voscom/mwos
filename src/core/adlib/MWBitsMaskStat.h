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
     * @param bitOffset   Номер бита. Если больше bitsCount то задать это значение всем битам
     */
    void setBit(bool value, uint16_t bitOffset) {
        if (bitOffset<bitsCount) {
            uint8_t inBytePos=bitOffset & 7;
            if (value) bitSet(bits[bitOffset >> 3],inBytePos);
            else bitClear(bits[bitOffset >> 3],inBytePos);
        } else {
            for (uint16_t i = 0; i < bitsCount; i++) {
                setBit(value,i);
            }
        }
    }

    /**
     * Прочитать бит
     * @param bitOffset
     * @return
     */
    bool getBit(uint16_t bitOffset) {
        if (bitOffset<bitsCount) return bitRead(bits[bitOffset >> 3],bitOffset);
        return false;
    }

};


#endif //MWOS3_MWOSBITSMASK_H
