#ifndef MWOS3_MWOSSTORAGERAM_H
#define MWOS3_MWOSSTORAGERAM_H

#include "core/MWStorageBytes.h"

const char MWOSStorageRAM_Name[] PROGMEM = {"RAM"};

/***
 * Бинарное хранилище в ОЗУ
 * Сбрасывается при перезагрузке и выключении
 * После создания можно явно задать буфер и смещение
 * Если буфер не задан, то при инициализации он будет создан автоматически в свободном ОЗУ
 */
class MWStorageRAM : public MWStorageBytes {
public:

    uint8_t * _buff=NULL;

    MWStorageRAM(char * unit_name=(char *) &MWOSStorageRAM_Name) : MWStorageBytes(unit_name) {
        MW_LOG_LN(F(" Inited!"));
    }

    virtual bool onInit(size_t bitSize, uint16_t crc16modules) {
        _bitSize=bitSize;
        if (_buff == NULL) { // если явно не задали буфер - создадим его
            _buff=new uint8_t[size()];
            _offset=0; // для автоматически созданного буфера нельзя задавать смещение
        }
        return MWStorageBytes::onInit(bitSize,crc16modules);
    }


    virtual uint8_t read(size_t byteOffset) {
        return _buff[byteOffset];
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        _buff[byteOffset]=v;
    }



};


#endif
