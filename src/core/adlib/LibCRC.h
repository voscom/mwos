/*
 * MWModule.h
 *      Author: mw
 *
 *  Библиотека для подсчета контрольных сумм CRC для ардуино
 *
 *
 *  постоянный расход ОЗУ: 2 байта
 */

#ifndef MWLIB_CRC_H_
#define MWLIB_CRC_H_

#include <Arduino.h>

/**
 * Популярные CRC
 * CRC8 - CRC-8/MAXIM (также известный как DOW-CRC или Dallas-Maxim)
 * CRC16 - CRC-16/MODBUS (или CRC-16/ARC). Полином 0xA001
 * CRC64 -  CRC-64/ISO (CRC-64-ECMA).
 */
class MW_CRC {
public:
    virtual void start() {
    }

    virtual void add(uint8_t dat) {
    }

    virtual uint64_t getCRC() {
        return 0;
    }

    void addInt16(uint16_t dat) {
        add((uint8_t) (dat & 0xff));
        add((uint8_t) ((dat >> 8) & 0xff));
    }

    void addString(const String &str) {
        for (uint8_t i=0; i<str.length(); i++) {
            add((uint8_t) str.charAt(i));
        }
    }

    void addBuffer(uint8_t * buffer, uint16_t size) {
        for (uint16_t i=0; i<size; i++) {
            add(buffer[i]);
        }
    }

    uint64_t calcBuffer(uint8_t * buffer, uint16_t size) {
        start();
        addBuffer(buffer,size);
        return getCRC();
    }

};


class MW_CRC16 : public MW_CRC {
public:

    MW_CRC16() {
        crc=0xFFFF;
    }

	virtual void start() {
		crc=0xFFFF;
	}

    virtual uint64_t getCRC() {
        return (uint64_t) crc;
    }

	virtual void add(uint8_t dat) {
		crc ^= (uint16_t) dat;    // XOR byte into least sig. byte of crc
		for (int i = 8; i>0; i--) {    // Loop over each bit
			if ((crc & 0x0001) != 0) {      // If the LSB is set
				crc >>= 1;                    // Shift right and XOR 0xA001
				crc ^= 0xA001;
		    }
			else                            // Else LSB is not set
		    	crc >>= 1;                    // Just shift right
	    }
	}

private:
    uint16_t crc;
};

/**
 * Для расчета используется (uint8_t) crc
 */
class MW_DS1W_CRC8: public MW_CRC {
public:

	MW_DS1W_CRC8() {
		crc=0;
	}

	virtual void start() {
		crc=0;
	}

    virtual uint64_t getCRC() {
        return (uint64_t) crc;
    }

	virtual void add(uint8_t dat) {
		uint8_t _crc=crc;
	    for(uint8_t i=0; i<8; i++) {  // счетчик битов в байте
	    	uint8_t fb = _crc ^ dat;
	    	fb &= 1;
	    	_crc >>= 1;
	    	dat >>= 1;
	    	if( fb == 1 ) _crc ^= 0x8c; // полином
	    }
	    crc=_crc;
	}

private:
    uint8_t crc;
};

class MW_CRC64: public MW_CRC {
public:

    MW_CRC64() {
        crc=0x0000000000000000;
        //count=0;
    }

    virtual void start() {
        crc=0x0000000000000000;
        //count=0;
    }

    virtual uint64_t getCRC() {
        return crc;
    }

    virtual void add(uint8_t dat) {
        //if (_reverseIn) dat = reverse8bits(dat);
        crc ^= ((uint64_t)dat) << 56;
        for (uint8_t i = 8; i>0; i--) {
            if ((crc & 0x8000000000000000)!=0) {
                crc <<= 1;
                crc ^= (uint64_t) 0x42F0E1EBA9EA3693;
            } else {
                crc <<= 1;
            }
        }
        //count++;
    }

private:
    uint64_t crc;

};

// ===== CRC-16 (Modbus/IBM, полином 0xA001) =====

// Инициализация: возвращает стартовое значение
inline uint16_t crc16_start() {
    return 0xFFFF;
}

// Добавление одного байта в расчёт CRC
inline uint16_t addCRC16(uint16_t crc, uint8_t dat) {
    crc ^= dat;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x0001) {
            crc = (crc >> 1) ^ 0xA001;
        } else {
            crc >>= 1;
        }
    }
    return crc;
}

// ===== Вспомогательные функции =====

// Добавление 16-битного значения (little-endian: младший байт первым)
inline uint16_t addCRC16_int16(uint16_t crc, uint16_t value) {
    crc = addCRC16(crc, (uint8_t)(value & 0xFF));
    crc = addCRC16(crc, (uint8_t)(value >> 8));
    return crc;
}

// Добавление строки Arduino String
inline uint16_t addCRC16_string(uint16_t crc, const String &str) {
    for (size_t i = 0; i < str.length(); i++) {
        crc = addCRC16(crc, (uint8_t)str.charAt(i));
    }
    return crc;
}

// Добавление буфера
inline uint16_t addCRC16_buffer(uint16_t crc, const uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        crc = addCRC16(crc, buffer[i]);
    }
    return crc;
}

// Полное вычисление CRC для буфера "с нуля"
inline uint16_t calcCRC16_buffer(const uint8_t *buffer, size_t size) {
    uint16_t crc = crc16_start();
    return addCRC16_buffer(crc, buffer, size);
}

// ===== CRC-8 Dallas/Maxim (1-Wire, полином 0x8C) =====

// Инициализация: стартовое значение
inline uint8_t crc8_ds1w_start() {
    return 0x00;
}

// Основная функция: добавление одного байта (та, что вы просили)
inline uint8_t addCRC8_ds1w(uint8_t crc, uint8_t dat) {
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t fb = (crc ^ dat) & 0x01;
        crc >>= 1;
        dat >>= 1;
        if (fb) crc ^= 0x8C;  // полином x^8 + x^5 + x^4 + x^0
    }
    return crc;
}

// ===== Вспомогательные функции =====

// Добавление 16-битного значения (little-endian: младший байт первым)
inline uint8_t addCRC8_ds1w_int16(uint8_t crc, uint16_t value) {
    crc = addCRC8_ds1w(crc, (uint8_t)(value & 0xFF));
    crc = addCRC8_ds1w(crc, (uint8_t)(value >> 8));
    return crc;
}

// Добавление строки Arduino String
inline uint8_t addCRC8_ds1w_string(uint8_t crc, const String &str) {
    for (size_t i = 0; i < str.length(); i++) {
        crc = addCRC8_ds1w(crc, (uint8_t)str.charAt(i));
    }
    return crc;
}

// Добавление буфера
inline uint8_t addCRC8_ds1w_buffer(uint8_t crc, const uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        crc = addCRC8_ds1w(crc, buffer[i]);
    }
    return crc;
}

// Полное вычисление CRC для буфера "с нуля"
inline uint8_t calcCRC8_ds1w_buffer(const uint8_t *buffer, size_t size) {
    uint8_t crc = crc8_ds1w_start();
    return addCRC8_ds1w_buffer(crc, buffer, size);
}

// ===== CRC-64 (ECMA-182, полином 0x42F0E1EBA9EA3693) =====

// Инициализация: стартовое значение
inline uint64_t crc64_start() {
    return 0x0000000000000000ULL;
}

// Основная функция: добавление одного байта (та, что вы просили)
inline uint64_t addCRC64(uint64_t crc, uint8_t dat) {
    crc ^= ((uint64_t)dat) << 56;  // XOR байта в старшие 8 бит
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x8000000000000000ULL) {
            crc = (crc << 1) ^ 0x42F0E1EBA9EA3693ULL;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

// ===== Вспомогательные функции =====

// Добавление 16-битного значения (big-endian: старший байт первым, как в оригинале)
inline uint64_t addCRC64_int16(uint64_t crc, uint16_t value) {
    crc = addCRC64(crc, (uint8_t)(value >> 8));   // старший байт
    crc = addCRC64(crc, (uint8_t)(value & 0xFF));  // младший байт
    return crc;
}

// Добавление 32-битного значения (big-endian)
inline uint64_t addCRC64_int32(uint64_t crc, uint32_t value) {
    crc = addCRC64(crc, (uint8_t)(value >> 24));
    crc = addCRC64(crc, (uint8_t)(value >> 16));
    crc = addCRC64(crc, (uint8_t)(value >> 8));
    crc = addCRC64(crc, (uint8_t)(value & 0xFF));
    return crc;
}

// Добавление 64-битного значения (big-endian)
inline uint64_t addCRC64_int64(uint64_t crc, uint64_t value) {
    for (int8_t i = 56; i >= 0; i -= 8) {
        crc = addCRC64(crc, (uint8_t)(value >> i));
    }
    return crc;
}

// Добавление строки Arduino String
inline uint64_t addCRC64_string(uint64_t crc, const String &str) {
    for (size_t i = 0; i < str.length(); i++) {
        crc = addCRC64(crc, (uint8_t)str.charAt(i));
    }
    return crc;
}

// Добавление буфера
inline uint64_t addCRC64_buffer(uint64_t crc, const uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        crc = addCRC64(crc, buffer[i]);
    }
    return crc;
}

// Полное вычисление CRC для буфера "с нуля"
inline uint64_t calcCRC64_buffer(const uint8_t *buffer, size_t size) {
    uint64_t crc = crc64_start();
    return addCRC64_buffer(crc, buffer, size);
}

#endif /* MWLIB_CRC_H_ */
