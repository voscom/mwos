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
#include "core/iodev/MWEEPROM.h"

class MW_CRC16 {
public:
	uint16_t crc=0xFFFF;

	virtual void start() {
		crc=0xFFFF;
	}

	virtual void addInt16(uint16_t dat) {
		add((uint8_t) (dat & 0xff));
		add((uint8_t) ((dat >> 8) & 0xff));
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

	uint16_t addString(const String &str) {
		for (uint8_t i=0; i<str.length(); i++) {
			add((uint8_t) str.charAt(i));
		}
		return crc;
	}

	uint16_t addBuffer(uint8_t * buffer, uint16_t size) {
		for (uint16_t i=0; i<size; i++) {
			add(buffer[i]);
		}
		return crc;
	}

	uint16_t calcBuffer(uint8_t * buffer, uint16_t size) {
		start();
		return addBuffer(buffer,size);
	}

	uint16_t calcEEPROM(uint16_t size, uint16_t offset=0) {
		start();
		for (uint16_t i=0; i<size; i++) {
			add(MWEEPROM.readByte(offset+i));
		}
		return crc;
	}
};

/**
 * Для расчета используется (uint8_t) crc
 */
class MW_DS1W_CRC8: public MW_CRC16 {
public:

	MW_DS1W_CRC8() {
		crc=0;
	}

	virtual void start() {
		crc=0;
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

};


#endif /* MWLIB_CRC_H_ */
