#ifndef MWOS3_MWOSSTORAGESTREAM_H
#define MWOS3_MWOSSTORAGESTREAM_H

/***
 * Универсальное бинарное хранилище со стандартным последовательным выводом
 * В качестве хранения - может использовать любых наследников MWOSStorageBytes
 *
 */

#include "core/MWStorageBytes.h"
#include "core/iodev/MWStreamAddr.h"

class MWStorageStream : public MWStorageBytes {
protected:
    MWStreamAddr * _stream;

public:

    MWStorageStream(MWStreamAddr * stream, char * unit_name) : MWStorageBytes(unit_name) {
        _stream=stream;
        if (_stream->begin()) {
            MW_LOG(F(" Stream size: ")); MW_LOG_LN((uint32_t) _stream->length());
        } else {
            MW_LOG_LN(F(" Error!"));
        }
    }

    virtual uint8_t read(size_t byteOffset) {
        uint8_t v=_stream->readByte(byteOffset);
#if defined(MWOS_LOG_STORAGE) && MWOS_LOG_STORAGE>1
        MW_LOG_TIME(); MW_LOG_PROGMEM(name); MW_LOG(F(" stream read "));  MW_LOG((uint32_t) byteOffset); MW_LOG('='); MW_LOG_LN(v);
#endif
        return v;
    }

    virtual void write(size_t byteOffset, uint8_t v) {
#if defined(MWOS_LOG_STORAGE) && MWOS_LOG_STORAGE>1
        MW_LOG_TIME(); MW_LOG_PROGMEM(name);  MW_LOG(F(" stream write "));  MW_LOG((uint32_t) byteOffset); MW_LOG('='); MW_LOG_LN(v);
#endif
        _stream->writeByte(byteOffset,v);
        _needCommit=true;
    }

    virtual bool commit(bool forced= false) {
        if (forced || _needCommit) {
            _stream->commit();
            MW_LOG_TIME(); MW_LOG_PROGMEM(name);  MW_LOG_LN(F(" stream commit!"));
        }
        return MWStorageBytes::commit(forced);
    }

};


#endif //MWOS3_MWOSSTORAGEI2C8_H
