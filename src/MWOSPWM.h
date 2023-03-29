#ifndef MWOS3_MWOSHZ_H
#define MWOS3_MWOSHZ_H
/**
 * Регулятор частоты и скваженности
 * Использует аппаратный ШИМ
 * Поддержка:
 * в папке platforms/MWOSHz_*
 *
 * Параметры:
 * 0 - 1=включено, 0=выключено
 * 1 - частота hz
 * 2 - скваженность %
 *
 * переназначает на модуль MWOSHz для конкретной платформы
 */
#include <Arduino.h>
#include "core/MWOSRegulator.h"
#ifdef STM32_MCU_SERIES
#include "platforms/MWOSHz_STM32.h"
#define MWOSHz MWOSHz_STM32
#endif
#ifdef ESP32
#include "platforms/MWOSPWM_ESP32.h"
#define MWOSPWM MWOSPWM_ESP32
#endif

#endif //MWOS3_MWOSHZ_H
