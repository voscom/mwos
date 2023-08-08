#ifndef MWOS3_MWOSSTORAGEESP32RTC_H
#define MWOS3_MWOSSTORAGEESP32RTC_H

#ifndef MWOSStorageEsp32RTC_SIZE
#define MWOSStorageEsp32RTC_SIZE 4096   // размер буффера
#endif

RTC_NOINIT_ATTR uint8_t MWOSStorageEsp32RTC_Buff[MWOSStorageEsp32RTC_SIZE]; // статический блок памяти в ESP32 RTC Slow memory

/***
 *
 * Бинарное хранилище в ESP32 RTC Slow memory
 * Позволяет динамическую перезапись
 * сбрасывается только при выключении питания, если нет резервной батареи
 *
 */
class MWOSStorageEsp32RTC : public MWOSStorage {
public:

    MWOSStorageEsp32RTC() : MWOSStorage() {
        MW_LOG(F("Storage ESP32 RTC Slow RAM size: ")); MW_LOG_LN(sizeof(MWOSStorageEsp32RTC_Buff));
    }

    virtual uint8_t read(size_t byteOffset) {
        return MWOSStorageEsp32RTC_Buff[byteOffset];
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        MWOSStorageEsp32RTC_Buff[byteOffset]=v;
    }


};


#endif //MWOS3_MWOSSTORAGEESPRTC_H
