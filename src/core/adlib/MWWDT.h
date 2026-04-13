#ifndef MWWDT_H_
#define MWWDT_H_
// ======== ESP32 ========
#if defined(ESP32)
#include <Arduino.h>
#include <esp_task_wdt.h>

// Наши константы (только если не AVR)
#ifndef WDTO_1S
#define WDTO_1S 1
#define WDTO_2S 2
#define WDTO_4S 4
#define WDTO_8S 8
#endif

void wdt_disable() {
    disableCore0WDT();
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3)
    disableCore1WDT();
#endif
    disableLoopWDT();
    esp_task_wdt_delete(NULL);
}

void wdt_enable(int wdtTimeout) {
    esp_task_wdt_init(wdtTimeout, true);
    esp_task_wdt_add(NULL);
}

void wdt_reset() {
    esp_task_wdt_reset();
}

// ======== STM32 ========
#elif defined(STM32_MCU_SERIES)
#include <Arduino.h>
#include <libmaple/iwdg.h>

#ifndef WDTO_1S
#define WDTO_1S 1
#define WDTO_2S 2
#define WDTO_4S 4
#define WDTO_8S 8
#endif

void wdt_disable() {
    // IWDG на STM32 нельзя отключить после запуска
}

void wdt_enable(int wdtTimeout) {
    uint32_t reload = (uint32_t)wdtTimeout * 156;
    if (reload > 4095) reload = 4095;  // Защита от переполнения
    iwdg_init(IWDG_PRE_256, reload);
}

void wdt_reset() {
    iwdg_feed();
}

// ======== AVR ========
#elif defined(ARDUINO_ARCH_AVR)
#include <Arduino.h>
#include <avr/wdt.h>

// На AVR используем нативные функции напрямую:
// wdt_enable(WDTO_1S), wdt_reset(), wdt_disable()
// Если нужна обёртка под секунды — см. пример выше с переименованием

// ======== Неизвестная платформа ========
#else
#pragma message "WDT: Platform not supported"
inline void wdt_disable() {}
inline void wdt_enable(int) {}
inline void wdt_reset() {}
#endif
#endif

