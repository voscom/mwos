#ifndef MWOS3_MWOSSTORAGEEEPROM_H
#define MWOS3_MWOSSTORAGEEEPROM_H

/***
 * Бинарное хранилище в EEPROM (или эмуляторе EEPROM в FLASH)
 * Может иметь довольно большой размер, но ограниченное количество перезаписей
 * Необходимо использовать для хранения параметров, которые не часто меняются
 */


#include "MWOSStorage.h"
#include <core/adlib/MWEEPROM.h>

#ifndef MWOS_STORAGE_EEPROM_OFFSET
#define MWOS_STORAGE_EEPROM_OFFSET 0 // смещение начала хранилища в EEPROM (если необходимо освободить начало EEPROM под другие нужды)
#endif

class MWOSStorageEEPROM : public MWOSStorage {
public:
    MWOSStorageEEPROM() : MWOSStorage() {
        bool present=MWEEPROM.begin();
        if (present) {
            MW_LOG(F("Storage EEPROM size: ")); MW_LOG_LN(MWEEPROM.length());
        } else {
            MW_LOG(F("Error Storage EEPROM:")); MW_LOG_LN(count);
        }
    }

    virtual uint8_t read(size_t byteOffset) {
        uint8_t v=MWEEPROM.read(byteOffset);
        MW_LOG(F("EEPROM read "));  MW_LOG(byteOffset); MW_LOG('='); MW_LOG_LN(v);
        return v;
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        MW_LOG(F("EEPROM write "));  MW_LOG(byteOffset); MW_LOG('='); MW_LOG_LN(v);
        MWEEPROM.write(byteOffset,v);
    }

    virtual void commit() {
        MWEEPROM.commit();
    }

};


#endif //MWOS3_MWOSSTORAGEEEPROM_H
