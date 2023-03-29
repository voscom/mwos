#ifndef MWOS3_MWOSSTORAGESTATICRAM_H
#define MWOS3_MWOSSTORAGESTATICRAM_H
/***
 * Динамическое бинарное хранилище в энергонезависимой памяти
 * Может быть разным, для разных микроконтроллеров
 * Позволяет хранить динамические данные с большим количеством перезаписей (в отличие от EEPROM и FLASH)
 * Например: текущие состояния ключей
 * Как правило, имеет небольшой размер. Это могут быть регистры или ОЗУ для RTC или BKP
 * Для хранения этих данных после выключения питания, как правило требуется резервная батарея
 *
 * Поддерживаемые микроконтроллеры:
 * STM32 - BKP REG (28x2=56b)
 * ESP8266 - RTC REG (128x4=512b)
 * ESP32 - RTC Slow RAM (4Kb) Размер можо уменьшить задав MWOSStorageEsp32RTC_SIZE меньше 4092
 *
 * Для остальных контроллеров создается хранилище в RAM (сбрасывается при перезагрузке)
 *
 **/

#include "MWOSStorage.h"

#ifdef STM32_MCU_SERIES
#include "core/hardware/platforms/MWOSStorageSTM32BKPReg.h"
#define MWOSStorageParentStaticRAM MWOSStorageSTM32BKPReg // для STM32 используем BKP-регистры
#endif

#ifdef ESP8266
#include "core/hardware/platforms/MWOSStorageEsp8266RTC.h"
#define MWOSStorageParentStaticRAM MWOSStorageEsp8266RTC // для ESP8266 используем RTC-память
#endif

#ifdef ESP32
#include "core/hardware/platforms/MWOSStorageEsp32RTC.h"
#define MWOSStorageParentStaticRAM MWOSStorageEsp32RTC // для ESP32 используем RTC slow RAM
#endif

#ifndef MWOSStorageParentStaticRAM
#include "MWOSStorageRAM.h"
#define MWOSStorageParentStaticRAM MWOSStorageRAM // для микроконтроллеров у корорых нет энергонезависимой памяти - используем обычную ОЗУ
#define MWOSStorageStaticRAM_NOT 1
#endif

class MWOSStorageStaticRAM : public MWOSStorageParentStaticRAM {
public:

    MWOSStorageStaticRAM() : MWOSStorageParentStaticRAM() {
    }


};

#endif // MWOS3_MWOSSTORAGESTATICRAM_H
