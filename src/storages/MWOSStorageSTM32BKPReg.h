#ifndef MWOS3_MWOSSTORAGEBKPREG_STM32_H
#define MWOS3_MWOSSTORAGEBKPREG_STM32_H
#ifdef STM32_MCU_SERIES
/***
 * Бинарное хранилище в STM32 BKP регистрах
 * Обычно для STM32 можно использовать 16-bit-регистры в диапазоне 0x0004 - 0x0028
 **/

#include "core/MWOSStorage.h"
#include <backup.h>

#ifndef MWOS_STORAGE_BKP_REGISTER_OFFSET
#define MWOS_STORAGE_BKP_REGISTER_OFFSET 4 // первый BKP регистра, для хранилища
#endif

const char MWOSStorageSTM32BKPReg_Name[] PROGMEM = {"STM32_BKP_REG"};

class MWOSStorageSTM32BKPReg : public MWOSStorage {
public:
    MWOSStorageSTM32BKPReg() : MWOSStorage() {
        setName((char *) &MWOSStorageSTM32BKPReg_Name);
    }

    virtual uint8_t read(size_t byteOffset) {
        enableBackupDomain();
        uint32_t regAddr=MWOS_STORAGE_BKP_REGISTER_OFFSET+(byteOffset>>1);
        uint16_t regData=getBackupRegister(regAddr);
        uint8_t v=0;
        if ((byteOffset & 1)>0) v= (regData >> 8) & 255;
        else v= regData & 255;
        MW_LOG(F("BKP reg read "));  MW_LOG(regAddr); MW_LOG('='); MW_LOG_LN(v, HEX);
        disableBackupDomain();
        return v;
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        enableBackupDomain();
        uint32_t regAddr=MWOS_STORAGE_BKP_REGISTER_OFFSET+(byteOffset>>1);
        uint16_t regData=getBackupRegister(regAddr);
        if ((byteOffset & 1)>0) regData = (regData & 0x00ff) + (((int16_t)v) << 8);
        else regData = (regData & 0xff00) + v;
        setBackupRegister(regAddr,regData);
        MW_LOG(F("BKP reg write "));  MW_LOG(regAddr); MW_LOG('='); MW_LOG_LN(regData,HEX);
        disableBackupDomain();
    }

};

#endif // STM32_MCU_SERIES
#endif // MWOS3_MWOSSTORAGEBKPREG_STM32_H
