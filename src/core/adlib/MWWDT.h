/**
 * WDT для платформ, где его нет
 * esp32
 * stm32
 *
 */

#ifndef MWWDT_H_
#define MWWDT_H_

#if defined(ESP32)
#include <Arduino.h>
#include "esp_system.h"
hw_timer_t * timerWDT = NULL;
void IRAM_ATTR resetModule() {
  //ets_printf("reboot wdt\n");
  esp_restart();
}
void wdt_disable () {
	if (timerWDT==NULL) return;
	timerWrite(timerWDT, 0);
	timerAlarmDisable(timerWDT);
	timerDetachInterrupt(timerWDT);
	timerEnd(timerWDT);
	timerWDT=NULL;
}
void wdt_enable (int wdtTimeout) {
	timerWDT = timerBegin(0, 80, true); //timer 0, div 80
	timerAttachInterrupt(timerWDT, &resetModule, true); //attach callback
	timerAlarmWrite(timerWDT, wdtTimeout * 1000, false); //set time in us
	timerAlarmEnable(timerWDT); //enable interrupt
}
void wdt_reset() {
	if (timerWDT!=NULL) timerWrite(timerWDT, 0); //reset timer (feed watchdog)
}
#endif


#if defined(STM32_MCU_SERIES)
#include <Arduino.h>
#include <libmaple/iwdg.h>
void wdt_disable () {
}
void wdt_enable (int wdtTimeout) {
	iwdg_init(IWDG_PRE_256, wdtTimeout*156);
}
void wdt_reset() {
	iwdg_feed();
}
#endif

#endif
