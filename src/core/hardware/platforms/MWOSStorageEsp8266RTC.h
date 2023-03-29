#ifndef MWOS3_MWOSSTORAGEESP8266RTC_H
#define MWOS3_MWOSSTORAGEESP8266RTC_H

/***
 *
 * Бинарное хранилище в ESP8266 RTC
 * Позволяет динамическую перезапись
 * Форматировано блоками по 32 бита
 * сбрасывается только при выключении питания, если нет резервной батареи
 *
 */

class MWOSStorageEsp8266RTC : public MWOSStorage {
public:
#ifdef ESP8266

    virtual uint8_t read(size_t byteOffset) {
        uint32_t regData=0;
        uint8_t v=0;
        if (ESP.rtcUserMemoryRead(byteOffset & 0b11111111111111111111111111111100, (uint32_t *)&regData, 4 )) {
            uint8_t inDWordOffset=byteOffset & 0b11;
            v=((uint8_t *)&regData)[inDWordOffset];
            MW_LOG(F("ESP RTC read "));  MW_LOG(byteOffset); MW_LOG('='); MW_LOG_LN(v, HEX);
        } else {
            MW_LOG(F("ESP RTC read error: "));  MW_LOG_LN(byteOffset);
        }
        return v;
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        uint32_t regData=0;
        uint8_t inDWordOffset=byteOffset & 0b11;
        if (ESP.rtcUserMemoryRead(byteOffset & 0b11111111111111111111111111111100, (uint32_t *)&regData, 4 )) {
            ((uint8_t *)&regData)[inDWordOffset]=v;
            if (ESP.rtcUserMemoryWrite(byteOffset & 0b11111111111111111111111111111100, (uint32_t *)&regData, 4)) {
                MW_LOG(F("ESP RTC write "));  MW_LOG(byteOffset); MW_LOG('='); MW_LOG_LN(v, HEX);
            } else {
                MW_LOG(F("ESP RTC write error: "));  MW_LOG_LN(byteOffset);
            }
        } else {
            MW_LOG(F("ESP RTC read error: "));  MW_LOG_LN(byteOffset);
        }
    }

#endif

};


#endif //MWOS3_MWOSSTORAGEESPRTC_H
