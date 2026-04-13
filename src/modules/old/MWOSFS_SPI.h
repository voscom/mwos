#ifndef MWOS3_MWOSFS_SPI_H
#define MWOS3_MWOSFS_SPI_H

#include "MWOSFS.h"

/**
 * Модуль работы с файловой системой через SPI
 *
 */
class MWOSFS_SPI : public MWOSFS {
public:

    /**
     * Порты SPI
     */
    MWOS_PARAM(4, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin+mwos_param_readonly, MWOS_STORAGE_NO, 4);

    MWOSFS_SPI() : MWOSFS() {
        AddParam(&p_pin);
    }

    MWOSFS_SPI(MWFS_SDCARD * _fs) : MWOSFS_SPI() {
        setFS(_fs);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param==&p_pin) {
            switch (arrayIndex) {
                case 0: return SCK;
                case 1: return MISO;
                case 2: return MOSI;
                case 3: return fs->pin;
            }
        }
        return MWOSFS::getValue(param, arrayIndex);
    }


};


#endif //MWOS3_MWOSFS_SPI_H
