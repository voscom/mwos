#ifndef MWOS3_MWOSSTORAGESTATICRAM_H
#define MWOS3_MWOSSTORAGESTATICRAM_H
/***
 * Универсальное динамическое бинарное хранилище в энергонезависимой памяти
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

#include "core/MWStorageBytes.h"

#ifdef STM32_MCU_SERIES
#include "storages/MWStorageSTM32BKPReg.h"
#define MWStorageParentStaticRAM MWStorageSTM32BKPReg // для STM32 используем BKP-регистры
#endif

#ifdef ESP8266
#include "storages/MWStorageEsp8266RTC.h"
#define MWStorageParentStaticRAM MWStorageEsp8266RTC // для ESP8266 используем RTC-память
#endif

#ifdef ESP32
#include "MWStorageEsp32RTC.h"
#define MWStorageParentStaticRAM MWStorageEsp32RTC // для ESP32 используем RTC slow RAM
#endif

#ifndef MWStorageParentStaticRAM
#include "MWStorageRAM.h"
#define MWStorageParentStaticRAM MWStorageRAM // для микроконтроллеров у корорых нет энергонезависимой памяти - используем обычную ОЗУ
#define MWStorageStaticRAM_NOT 1
#endif

class MWStorageStaticRAM : public MWStorageParentStaticRAM {
public:

    MWStorageStaticRAM() : MWStorageParentStaticRAM() {
    }


};

#endif
