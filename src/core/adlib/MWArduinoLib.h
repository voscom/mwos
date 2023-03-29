#ifndef MWOS3_MWARDUINOLIB_H
#define MWOS3_MWARDUINOLIB_H
/***
 * Содержит некоторые полезные функции для работы с платформой Arduino на разных микроконтроллерах
 */

#include "Arduino.h"

#ifdef AVR
extern int __bss_end;
extern void *__brkval;
#endif
#ifdef STM32_MCU_SERIES
#include <malloc.h>
extern "C" char *sbrk(int i);
/* Use linker definition */
extern char _end;
extern char _sdata;
extern char _estack;
extern char _Min_Stack_Size;

static char *ramstart = &_sdata;
static char *ramend = &_estack;
static char *minSP = (char*)(ramend - &_Min_Stack_Size);
#endif

uint32_t getFreeMemory() {
    uint32_t freeValue=0;
#ifdef ESP8266
    freeValue=ESP.getFreeHeap();
#endif
#ifdef ESP32
    freeValue=ESP.getFreeHeap();
#endif
#ifdef STM32_MCU_SERIES
    char *heapend = (char*)sbrk(0);
    char * stack_ptr = (char*)__get_MSP();
    struct mallinfo mi = mallinfo();
	freeValue=((stack_ptr < minSP) ? stack_ptr : minSP) - heapend + mi.fordblks;
#endif
#ifdef AVR
    if((int)__brkval == 0) freeValue = ((int)&freeValue) - ((int)&__bss_end);
	else freeValue = ((int)&freeValue) - ((int)__brkval);
#endif
    return freeValue;
}

void resetController() {
#if defined(ESP8266) || defined(ESP32)
    ESP.restart();
#endif
#ifdef STM32_MCU_SERIES
    NVIC_SystemReset();
#endif
}

uint32_t getChipID() {
    uint32_t chipID=0;
#ifdef ESP8266
    chipID=ESP.getChipId();
#endif
#ifdef ESP32
    chipID=(uint32_t) ESP.getEfuseMac();
#endif
#ifdef STM32_MCU_SERIES
    uint32_t *idBase3 =  (uint32_t *) (0x1FFFF7E8+0x08);
		chipID=*(idBase3);
#endif
    return chipID;
}


uint8_t getPlatform() {
    uint8_t pl=0;
#ifdef STM32_MCU_SERIES
    pl=33;
#endif
#ifdef __STM32F1__
    pl=34;
#endif
#ifdef ESP8266
    pl= 60;
#endif
#ifdef ESP32
    pl= 64;
#endif
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
    pl= 1;
#endif
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2560P__)
    pl= 2;
#endif
#if defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168__)
    pl= 3;
#endif
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1280P__)
    pl= 4;
#endif
    return pl;
}


#endif //MWOS3_MWARDUINOLIB_H
