#ifndef MWOS3_MWOSSTORAGEEEPROM_H
#define MWOS3_MWOSSTORAGEEEPROM_H

/***
 * Бинарное хранилище в EEPROM (или эмуляторе EEPROM в FLASH)
 * Может иметь довольно большой размер, но ограниченное количество перезаписей
 * Необходимо использовать для хранения параметров, которые не часто меняются
 */

#include "core/iodev/MWEEPROM.h"
#include "core/MWStorageStream.h"

class MWStorageEEPROM : public MWStorageStream {
public:
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
    MWStorageEEPROM() : MWStorageStream(&MWEEPROM,(char *) F("EEPROM")) { // зададим EEPROM для хранилища
        _offset=MWOS_STORAGE_EEPROM_OFFSET+4; // первые 4 байта под CID
    }
#endif

};


#endif
