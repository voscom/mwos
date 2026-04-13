/*
 * autor: Vladimir Monakhov <vladimir9@bk.ru>
 *
 * Универсальный класс EEPROM
 * Одинаково работает на всех платформах
 * На платформах, где нет EEPROM, использует раздел флеш-памяти для эмуляции (если это возможно)
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

#ifndef MWOS_STORAGE_EEPROM_OFFSET
// Смещение начала хранилища в EEPROM (если необходимо освободить начало EEPROM под другие нужды).
// По этому адресу хранится CID, и далее - хранилище в EEPROM.
#define MWOS_STORAGE_EEPROM_OFFSET 0
#endif

#include "MWStreamAddr.h"

class MWEEPROMClass : public MWStreamAddr {
public:
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)

    virtual bool begin() {
    	if (inited) return true;
#if defined(ESP8266) || defined(ESP32)
        // MW_LOG_TIME(); MW_LOG(F("Inited ESP EEPROM: ")); MW_LOG_LN(MWOS_ESP_EEPROM_SIZE);
    	EEPROM.begin(MWOS_ESP_EEPROM_SIZE);
#endif
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        eeprom_buffer_fill();  // Copy the data from the flash to the buffer
#endif
#endif
    	return MWStreamAddr::begin();
    }

    virtual bool end() {
#if defined(ESP8266) || defined(ESP32)
    	EEPROM.end();
#endif
    	return MWStreamAddr::end();
    }

    virtual int available() { return 0; }
    virtual int peek() { return 0; }

    void clear(uint16_t addr=0) {
    	if (!inited) begin();
		writeWord(addr, 0xffff); // очистим первые 2 байта - этого достаточно
		commit();
    }

    virtual bool commit() {
#if defined(ESP8266) || defined(ESP32)
		return EEPROM.commit();
#endif
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        eeprom_buffer_flush(); // Copy the data from the buffer to the flash
#endif
#endif
        return MWStreamAddr::commit();
	}

    virtual size_t length() {
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

    virtual int read() {
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        return eeprom_buffered_read_byte(addr++);
#else
        return eeprom_read_byte(addr++);
#endif
#else
    	return EEPROM.read(addr++);
#endif
    }

    virtual size_t write(uint8_t val) {
#ifdef STM32_MCU_SERIES
#ifndef DATA_EEPROM_BASE
        eeprom_buffered_write_byte(addr++,val);
#else
        eeprom_write_byte(addr++,val);
#endif
#else
    	EEPROM.write(addr++,val);
#endif
        return 1;
    }
#endif

};
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
MWEEPROMClass MWEEPROM;
#endif

#endif
