#ifndef MWOS3_MWOSSTORAGEEEPROM_H
#define MWOS3_MWOSSTORAGEEEPROM_H

/***
 * Бинарное хранилище в EEPROM (или эмуляторе EEPROM в FLASH)
 * Может иметь довольно большой размер, но ограниченное количество перезаписей
 * Необходимо использовать для хранения параметров, которые не часто меняются
 */

#include "MWOSStorageStream.h"
#include "core/iodev/MWEEPROM.h"

#ifndef MWOS_STORAGE_EEPROM_OFFSET
#define MWOS_STORAGE_EEPROM_OFFSET 32 // смещение начала хранилища в EEPROM (если необходимо освободить начало EEPROM под другие нужды)
#endif

const char MWOSStorageEEPROM_Name[] PROGMEM = {"EEPROM"};

class MWOSStorageEEPROM : public MWOSStorageStream {
public:
    MWOSStorageEEPROM() : MWOSStorageStream(&MWEEPROM) { // зададим EEPROM для хранилища
        _offset=MWOS_STORAGE_EEPROM_OFFSET;
        setName((char *) &MWOSStorageEEPROM_Name);
        MW_LOG_LN(F(" - storage EEPROM"));
    }


};


#endif //MWOS3_MWOSSTORAGEEEPROM_H
