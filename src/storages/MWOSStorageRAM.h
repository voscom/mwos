#ifndef MWOS3_MWOSSTORAGERAM_H
#define MWOS3_MWOSSTORAGERAM_H

const char MWOSStorageRAM_Name[] PROGMEM = {"RAM"};

/***
 * Бинарное хранилище в ОЗУ
 * Сбрасывается при перезагрузке и выключении
 * После создания можно явно задать буффер и смещение
 * Если буффер не задан, то при инициализации он будет создан автоматически в свободном ОЗУ
 */
class MWOSStorageRAM : public MWOSStorage {
public:

    uint8_t * buff=NULL;

    MWOSStorageRAM() : MWOSStorage() {
        setName((char *) &MWOSStorageRAM_Name);
        MW_LOG(F("Storage RAM: ")); MW_LOG_LN(count);
    }

    virtual bool onInit(size_t bitSize, uint16_t crc16modules) {
        if (buff==NULL) { // если явно не задали буффер - создадим его
            uint32_t byteSize=bitSize >> 3;
            if ((bitSize & 7) >0) byteSize++;
            //byteSize+=CheckSumSize;
            buff=new uint8_t[byteSize];
            _offset=0; // для автоматически созданного буффера нельзя задавать смещение
        }
        return MWOSStorage::onInit(bitSize,crc16modules);
    }


    virtual uint8_t read(size_t byteOffset) {
        return buff[byteOffset];
    }

    virtual void write(size_t byteOffset, uint8_t v) {
        buff[byteOffset]=v;
    }



};


#endif //MWOS3_MWOSSTORAGERAM_H
