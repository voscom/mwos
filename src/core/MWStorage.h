#ifndef MWOS3_MWOSSTORAGEBASE_H
#define MWOS3_MWOSSTORAGEBASE_H


#include "core/MWOSConsts.h"
#include "MWOSDebug.h"
#include "MWOSUnitName.h"

/**
 * Базовый класс работы с битовым хранилищем
 * Сначала идут все битовые параметры, а от _offsetByteSide - байтовые
 *
 * Необходимо переопределить saveValue и loadValue
 * может сохранять массив битовых значений в хранилище
 */
class MWStorage : public MWOSUnitName {
public:
    static uint8_t storagesCount; // количество созданных хранилищ
#pragma pack(push,1)
    uint8_t _num; // номер хранилища
    uint8_t _checkSumSize:4; // размер контрольной суммы хранилища (в байтах)
    uint8_t _inited:1; // признак, что контрольная сумма хранилища сохранена
    uint8_t _needCommit:1; // признак, что необходим комит
    uint8_t :2;
    mwos_size _bitSize=0;
    mwos_size _offset=0; // смещение начала хранилища (в байтах)
    mwos_size _offsetBitsByteSide=0; // смещение начала байтовой части хранилища (в битах), относительно начала хранилища (до этого идет битовая часть хранилища)
#pragma pack(pop)

    MWStorage(char * unit_name=NULL) : MWOSUnitName(unit_name) {
        MW_LOG_TIME(); MW_LOG_PROGMEM(name);
        _num=storagesCount;
        storagesCount++;
        _checkSumSize=0;
        _inited=false;
        _needCommit=false;
    }

    /**
     * Актуальный размер хранилища в байтах
     * @return
     */
    uint32_t size() {
        uint32_t byteSize=_bitSize >> 3;
        if ((_bitSize & 7) >0) byteSize++;
        return byteSize;
    }

    /**
     * Вызывается при инициализации MWOS
     * @param bitSize       Размер хранилища (бит)
     * @param crc16modules  Контрольная сумма хранилища
     * @return
     */
    virtual bool onInit(mwos_size bitSize, uint16_t crc16modules) {
        _bitSize=bitSize;
        MW_LOG_TIME(); MW_LOG(F("onInit storage ")); MW_LOG(_num); MW_LOG(':'); MW_LOG_PROGMEM(name); MW_LOG('='); MW_LOG_LN((uint32_t) _bitSize);
        return false;
    }

    /***
     * Сохранить контрольную сумму хранилища. После этого хранилище считается инициализированным.
     * Не использовать напрямую! Вызывать только через mwos.initStorage
     * @param crc16modules  crc16 из mwos.crc16modules
     */
    virtual void initStorage(uint16_t crc16modules) {
        _inited=true;
        MW_LOG_TIME(); MW_LOG(F("Storage inited CRC ")); MW_LOG(_num); MW_LOG(':'); MW_LOG_PROGMEM(name); MW_LOG('='); MW_LOG_LN(crc16modules);
    }

    /***
     * Сохранить массив значений в хранилище
     * @param dataArray Массив значений
     * @param bitOffset Смещение (бит) в хранилище
     * @param bitSize   Размер в битах
     * @param maxByteSize   Ограничение на размер в хранилище (в байтах)
     * @return Сколько байт записано
     */
    virtual mwos_size saveValue(uint8_t * dataArray, mwos_size bitOffset, mwos_size bitSize, mwos_size maxByteSize=0) {
        return false;
    }


    /**
     * Прочитать в буфер значение
     * @param buff      Буфер для чтения
     * @param bitOffset Смещение (бит) в хранилище
     * @param bitSize   Размер в битах
     * @param maxByteSize   Ограничение на размер в хранилище (в байтах)
     * @return Считано байт
     */
    virtual mwos_size loadValue(uint8_t * buff, mwos_size bitOffset, mwos_size bitSize, mwos_size maxByteSize=0) {
        return 0;
    }

    /**
     * Комит изменений в хранилище
     * Вызывать из потомков вначале переопределения!
     * @param forced    Сохранить в любом случае (иначе, только если были изменения)
     * @return  Было сохранение
     */
    virtual bool commit(bool forced= false) {
        if (_needCommit==0 && forced==0) return false;
        _needCommit=false;
        MW_LOG_TIME(); MW_LOG(_num); MW_LOG(':'); MW_LOG_PROGMEM(name); MW_LOG_LN(F(" - commit storage"));
        return true;
    }


};
uint8_t MWStorage::storagesCount=0;


#endif //MWOS3_MWOSSTORAGE_H
