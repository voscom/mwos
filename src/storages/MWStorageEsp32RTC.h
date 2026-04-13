#ifndef MWOS3_MWOSSTORAGEESP32RTC_H
#define MWOS3_MWOSSTORAGEESP32RTC_H

#include "core/MWStorageBytes.h"

#ifndef MWOSStorageEsp32RTC_SIZE
#define MWOSStorageEsp32RTC_SIZE 256   // размер буфера в ESP32 RTC Slow memory
#endif

RTC_NOINIT_ATTR static uint8_t MWOSStorageEsp32RTC_Buff[MWOSStorageEsp32RTC_SIZE]; // статический блок памяти в ESP32 RTC Slow memory

/***
 *
 * Бинарное хранилище в ESP32 RTC Slow memory
 * Позволяет динамическую перезапись
 * сбрасывается только при выключении питания, если нет резервной батареи
 *
 */
class MWStorageEsp32RTC : public MWStorageBytes {
public:

    MWStorageEsp32RTC() : MWStorageBytes((char *) F("ESP32_RTC")) {
        MW_LOG(F(" ESP32 RTC RAM size: ")); MW_LOG_LN(sizeof(MWOSStorageEsp32RTC_Buff));
        //_cacheWriteBit=false; // не кешировать побитную запись
    }

    virtual uint8_t read(size_t byteOffset) {
        return MWOSStorageEsp32RTC_Buff[byteOffset];
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        MWOSStorageEsp32RTC_Buff[byteOffset]=v;
    }


};


#endif
