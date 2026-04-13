#ifndef MWOS3_MWARDUINOLIB_H
#define MWOS3_MWARDUINOLIB_H
/***
 * Содержит некоторые полезные функции для работы с платформой Arduino на разных микроконтроллерах
 */

#include "Arduino.h"

// Используем стандартные макросы архитектуры
#if defined(ARDUINO_ARCH_AVR)
    extern int __bss_end;
    extern void *__brkval;
#elif defined(STM32_MCU_SERIES)
    #include <malloc.h>
    extern "C" char *sbrk(int i);
    extern char _end;
    extern char _sdata;
    extern char _estack;
    extern char _Min_Stack_Size;
    static char *ramstart = &_sdata;
    static char *ramend = &_estack;
    static char *minSP = (char*)(ramend - &_Min_Stack_Size);
#endif

// Добавлено inline для предотвращения ошибок линковки
inline uint32_t getFreeMemory() {
    uint32_t freeValue = 0;

#if defined(ESP8266) || defined(ESP32)
    freeValue = ESP.getFreeHeap();

#elif defined(STM32_MCU_SERIES)
    char *heapend = (char*)sbrk(0);
    char * stack_ptr = (char*)__get_MSP();
    struct mallinfo mi = mallinfo();
    freeValue = ((stack_ptr < minSP) ? stack_ptr : minSP) - heapend + mi.fordblks;

#elif defined(ARDUINO_ARCH_AVR)
    if((int)__brkval == 0)
        freeValue = ((int)&freeValue) - ((int)&__bss_end);
    else
        freeValue = ((int)&freeValue) - ((int)__brkval);
#endif
    return freeValue;
}

inline void resetController() {
#if defined(ESP8266) || defined(ESP32)
    ESP.restart();
#elif defined(STM32_MCU_SERIES)
    NVIC_SystemReset();
#else
    // Fallback для других платформ (перезагрузка через указатель на 0 - опасно, но работает на многих)
    // Или просто while(1);
    void (*resetFunc)(void) = 0;
    resetFunc();
#endif
}

inline uint32_t getChipID() {
    uint32_t chipID = 0;

#ifdef MWOS_CHIP_ID
    chipID = MWOS_CHIP_ID;
#else
    #if defined(ESP8266)
        chipID = ESP.getChipId();
    #elif defined(ESP32)
        // getEfuseMac возвращает 64 бит, берем младшие 32
        chipID = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
    #elif defined(STM32_MCU_SERIES)
        uint32_t *idBase3 = (uint32_t *)(0x1FFFF7E8 + 0x08);
        chipID = *(idBase3);
    #elif defined(ARDUINO_ARCH_AVR)
        // У простых AVR нет уникального серийного номера в стандартном доступе
        chipID = 0;
    #endif
#endif
    return chipID;
}

#endif
