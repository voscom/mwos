#ifndef MWOS3_MWOSSTORAGESTREAM_H
#define MWOS3_MWOSSTORAGESTREAM_H

/***
 * Универсальное бинарное хранилище со стандартным последовательным выводом
 * В качестве хранения - может использовать любых наследников MWOSStorage
 *
 */

#include "../core/MWOSStorage.h"
#include "core/iodev/MWEEPROM.h"
#include <Wire.h>

class MWOSStorageStream : public MWOSStorage {
protected:
    MWStream * _stream;

public:

    MWOSStorageStream(MWStream * stream) : MWOSStorage() {
        _stream=stream;
        if (_stream->begin()) {
            MW_LOG_PROGMEM(name);  MW_LOG(F(" Storage stream size: ")); MW_LOG_LN((uint32_t) _stream->length());
        } else {
            MW_LOG_PROGMEM(name);  MW_LOG(F(" Error Storage stream:")); MW_LOG_LN(count);
        }
    }

    virtual uint8_t read(size_t byteOffset) {
        uint8_t v=_stream->readByte(byteOffset);
        MW_LOG_PROGMEM(name); MW_LOG(F(" stream read "));  MW_LOG((uint32_t) byteOffset); MW_LOG('='); MW_LOG_LN(v);
        return v;
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        MW_LOG_PROGMEM(name);  MW_LOG(F(" stream write "));  MW_LOG((uint32_t) byteOffset); MW_LOG('='); MW_LOG_LN(v);
        _stream->writeByte(byteOffset,v);
    }

    virtual bool commit(bool forced= false) {
        if (!MWOSStorage::commit(forced)) return false;
        _stream->commit();
        MW_LOG_PROGMEM(name);  MW_LOG_LN(F(" stream commit!"));
        return true;
    }

};


#endif //MWOS3_MWOSSTORAGEI2C8_H
