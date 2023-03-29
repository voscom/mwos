/*
 * autor: Vladimir Monakhov <vladimir9@bk.ru>
 *
 * Универсальный класс EEPROM
 * Одинаково работает на всех платформах
 * На платформах, где нет EEPROM, использует раздел флеш-памяти для эмуляции
 * Поддерживаемые платформы:
 * Arduino
 * stm32	Поддерживает:
 *              Generic STM32 HAL driver for EEPROM devices (по адресу DATA_EEPROM_BASE и до DATA_EEPROM_END)
 *              запись в блок PROGRAM FLASH (по адресу FLASH_BASE_ADDRESS)
 * esp8266
 * esp32
 *
 */

#ifndef MWEEPROM_H_
#define MWEEPROM_H_

//#define NO_GLOBAL_EEPROM

#include <Arduino.h>
#ifdef STM32_MCU_SERIES
#include "utility/stm32_eeprom.h"
#else
#include <EEPROM.h>
#endif

#ifndef MWOS_ESP_EEPROM_SIZE
#define MWOS_ESP_EEPROM_SIZE 2048
#endif

class MWEEPROMClass {
public:

	bool inited=false;

	MWEEPROMClass() {
	}
	virtual ~MWEEPROMClass() {};

    template<typename T>
    T &get(int address, T &t) {
      return writeBuffer(address, (uint8_t*) &t, sizeof(T));
    }

    template<typename T>
    const T &put(int address, const T &t) {
       return readBuffer(address, (uint8_t*) &t, sizeof(T));
    }

    bool begin() {
    	if (inited) return true;
    	inited=true;
#if defined(ESP8266) || defined(ESP32)
		MW_LOG(F("Inited EEPROM: ")); MW_LOG(MWOS_ESP_EEPROM_SIZE); MW_LOG(F("b, "));  MW_DEBUG_LOG_MEM(false);
    	EEPROM.begin(MWOS_ESP_EEPROM_SIZE);
#endif
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        eeprom_buffer_fill();  // Copy the data from the flash to the buffer
#endif
#endif
    	return inited;
    }

    void end() {
#if defined(ESP8266) || defined(ESP32)
    	EEPROM.end();
#endif
    }

    void clear() {
    	if (!inited) begin();
		write(0, 255); // очистим первые 2 байта - этого достаточно
        write(1, 255);
		commit();
    }

    void commit() {
#if defined(ESP8266) || defined(ESP32)
		EEPROM.commit();
#endif
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        eeprom_buffer_flush(); // Copy the data from the buffer to the flash
#endif
#endif
	}

    uint16_t length() {
#ifdef STM32_MCU_SERIES
#ifdef DATA_EEPROM_BASE
        return FLASH_PAGE_SIZE/8;
#else
        return FLASH_PAGE_SIZE;
#endif
#else
#if defined(ESP8266) || defined(ESP32)

#else
		return EEPROM.length();
#endif
#endif
		return 2048;
    }

    uint8_t read(uint16_t address) {
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        return eeprom_buffered_read_byte(address);
#else
        return eeprom_read_byte(address);
#endif
#else
    	return EEPROM.read(address);
#endif
    }

    void write(uint16_t address, uint8_t val) {
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        eeprom_buffered_write_byte(address,val);
#else
        eeprom_write_byte(address,val);
#endif
#else
    	EEPROM.write(address,val);
#endif
    }

    uint16_t readBuffer(uint16_t address, uint8_t * buffer, uint16_t size) {
    	for (uint16_t i=0; i<size; i++) buffer[i]=read(address+i);
    	return size;
    }

    uint16_t writeBuffer(uint16_t address, uint8_t * buffer, uint16_t size) {
    	for (uint16_t i=0; i<size; i++) write(address+i,buffer[i]);
    	return size;
    }

    uint16_t readWord(uint16_t address) {
    	return read(address)+(((uint16_t) read(address+1)) << 8);
    }

    uint32_t readDWord(uint16_t address) {
    	return readWord(address)+(((uint32_t) readWord(address+2)) << 16);
    }

    void writeWord(uint16_t address, uint16_t val) {
    	write(address,val & 0xff);
    	write(address+1,(val >> 8) & 0xff);
    }

    void writeDWord(uint16_t address, uint32_t val) {
    	writeWord(address,val & 0xffff);
    	writeWord(address+2,(val >> 16) & 0xffff);
    }

};
MWEEPROMClass MWEEPROM;

#endif
