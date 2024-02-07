/**
 * Базовый класс операционной системы MWOS3
 *
 */
#ifndef MWOS3_MWOS_H
#define MWOS3_MWOS_H

#include <Arduino.h>
#include "core/MWOSConsts.h"
#include "core/adlib/MWWDT.h"

#ifndef STM32_MCU_SERIES
#ifdef STM32_CORE_VERSION
#define STM32_MCU_SERIES
#endif
#ifdef STM32F4xx
#define STM32_MCU_SERIES
#endif
#endif

#include "core/MWOS3.h"
#include "core/net/MWOSModuleLog.h"
#ifdef ESP8266
#include <MWOSNetWiFi.h>
#endif
#ifdef ESP32
#include <MWOSNetWiFi.h>
#endif


void mwos_start() {
    mwos.start();
}

void mwos_update() {
    mwos.update();
}

#endif //MWOS3_MWOS_H
