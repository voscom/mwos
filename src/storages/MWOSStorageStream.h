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
            MW_LOG(F("Storage stream size: ")); MW_LOG_LN(_stream->length());
        } else {
            MW_LOG(F("Error Storage stream:")); MW_LOG_LN(count);
        }
    }

    virtual uint8_t read(size_t byteOffset) {
        uint8_t v=_stream->readByte(byteOffset);
        MW_LOG(F("stream read "));  MW_LOG(byteOffset); MW_LOG('='); MW_LOG_LN(v);
        return v;
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        MW_LOG(F("EEPROM write "));  MW_LOG(byteOffset); MW_LOG('='); MW_LOG_LN(v);
        _stream->writeByte(byteOffset,v);
    }

    virtual void commit() {
        _stream->commit();
    }

};


#endif //MWOS3_MWOSSTORAGEI2C8_H
