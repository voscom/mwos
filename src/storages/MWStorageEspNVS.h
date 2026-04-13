#ifndef MWOS3_MWOSSTORAGEESPNVS_H
#define MWOS3_MWOSSTORAGEESPNVS_H


#include <nvs_flash.h>
#include "core/MWOSConsts.h"
#include "core/MWStorage.h"
#include "nvs.h"

/**
 * Хранилище в ESP NVS
 * Подходит для хранения строк и небольших массивов
 * предполагает, что уже сделано nvs_flash_init
 *
 * Использует битовое смещение, как ключ для значений
 *
 */
class MWStorageEspNVS : public MWStorage {
public:

    nvs_handle_t nvs_handle;

    MWStorageEspNVS() : MWStorage((char *) F("EspNVS")) {
#ifdef MWOS_NVS_PARTITION
        nvs_flash_init_partition(MWOS_NVS_PARTITION);
#endif
        MW_LOG_LN(F(" Inited!"));
    }

    /**
     * Вызывается при инициализации MWOS
     * @param bitSize       Размер хранилища (бит)
     * @param crc16modules  Контрольная сумма хранилища
     * @return
     */
    virtual bool onInit(mwos_size bitSize, uint16_t crc16modules) {
#ifdef MWOS_NVS_PARTITION
        esp_err_t err = nvs_open_from_partition(MWOS_NVS_PARTITION,"mwos", NVS_READWRITE, &nvs_handle);
#else
        esp_err_t err1 = nvs_open("mwos", NVS_READWRITE, &nvs_handle);
#endif
        if (err1 == ESP_OK) {
            uint16_t savedCRC = 0;
            esp_err_t err = nvs_get_u16(nvs_handle, "crc16modules", &savedCRC);
            if (err == ESP_OK) _inited=(crc16modules == savedCRC);
            MWStorage::onInit(bitSize,crc16modules);
            MW_LOG_TIME(); MW_LOG(F("onInit EspNVS CRC: ")); MW_LOG(_inited); MW_LOG(';'); MW_LOG(crc16modules); MW_LOG('='); MW_LOG_LN(savedCRC);
            return _inited;
        }
        MW_LOG_TIME(); MW_LOG(F("onInit EspNVS partition error: "));  MW_LOG_LN((int) err1);
        return false;
    }

    /***
     * Сохранить контрольную сумму хранилища. После этого хранилище считается инициализированным.
     * Не использовать напрямую! Вызывать только через mwos.initStorage
     * @param crc16modules  crc16 из mwos.crc16modules
     */
    virtual void initStorage(uint16_t crc16modules) {
        esp_err_t err = nvs_set_u16(nvs_handle, "crc16modules", crc16modules);
        if (err == ESP_OK) {
            MWStorage::initStorage(crc16modules);
            return;
        }
        MW_LOG_TIME(); MW_LOG(F("initStorage EspNVS error: ")); MW_LOG((int) err); MW_LOG(';'); MW_LOG_LN((uint32_t) _bitSize);
    }

    /***
     * Сохранить массив значений в хранилище
     * @param dataArray Массив значений
     * @param bitOffset Смещение (бит) в хранилище
     * @param bitSize   Размер в битах
     * @param maxByteSize   Ограничение на размер в хранилище (в байтах). Для строки всегда 0
     * @return Сколько байт записано
     */
    virtual mwos_size saveValue(uint8_t * dataArray, mwos_size bitOffset, mwos_size bitSize, mwos_size maxByteSize=0) {
        //if (!_inited) return 0;
        if (!bitSize) return 0;
        uint16_t byteSize=bitSize >> 3;
        if ((bitSize & 7)>0) byteSize++;
        if (maxByteSize>0 && byteSize>maxByteSize) byteSize=maxByteSize;
        char keyChars[10];
        int2chars((char *) &keyChars,(uint16_t) bitOffset);
        esp_err_t err;
        if (maxByteSize==0) { // это строка
            byteSize=strlen((char *) dataArray)+1; // длина строки и 0 в конце
        }
        err = nvs_set_blob(nvs_handle,(char *) &keyChars,dataArray,byteSize);
#ifdef MWOS_LOG_STORAGE
        MW_LOG_TIME(); MW_LOG(F("NVS save ")); MW_LOG((char *) keyChars); MW_LOG('=');
#endif
        if (err == ESP_OK) {
#ifdef MWOS_LOG_STORAGE
            if (maxByteSize==0) { MW_LOG((char *) dataArray); }
            else { MW_LOG_BYTES(dataArray,byteSize); }
            MW_LOG_LN();
#endif
            _needCommit= true;
            return byteSize;
        }

#ifdef MWOS_LOG_STORAGE
        MW_LOG(F("error code ")); MW_LOG_LN((int32_t) err);
#endif
        return 0;
    }

    /**
     * Прочитать в буфер значение
     * @param buff      Буфер для чтения
     * @param bitOffset Смещение (бит) в хранилище
     * @param bitSize   Размер в битах (для строки - это максимальная длина)
     * @param maxByteSize   Ограничение на размер в хранилище (в байтах). Для строки всегда 0
     * @return Считано байт
     */
    virtual mwos_size loadValue(uint8_t * buff, mwos_size bitOffset, mwos_size bitSize, mwos_size maxByteSize=0) {
        if (!_inited) return 0;
        size_t byteSize=bitSize>>3; if ((bitSize & 7)>0) byteSize++;
        char keyChars[10];
        int2chars((char *) &keyChars,(uint16_t) bitOffset);
        esp_err_t err;
        err = nvs_get_blob(nvs_handle, (char *) &keyChars, buff, &byteSize);
#ifdef MWOS_LOG_STORAGE
        MW_LOG_TIME(); MW_LOG(F("NVS load ")); MW_LOG((char *) keyChars); MW_LOG('=');
#endif
        if (err == ESP_OK) {
#ifdef MWOS_LOG_STORAGE
            MW_LOG_BYTES(buff,byteSize); MW_LOG_LN();
#endif
            return byteSize;
        }
#ifdef MWOS_LOG_STORAGE
        MW_LOG(F("error code ")); MW_LOG_LN((int32_t) err);
#endif
        return 0;
    }

    /**
     * Комит изменений в хранилище
     * Вызывать из потомков вначале переопределения!
     * @param forced    Сохранить в любом случае (иначе, только если были изменения)
     * @return  Было сохранение
     */
    virtual bool commit(bool forced= false) {
        if (forced || _needCommit) nvs_commit(nvs_handle);
        return MWStorage::commit(forced);
    }

protected:

    /**
     * Перевести целое число в строку вида "k233" 0 в конце
     * @param buff  Куда записывать символы
     * @param a     Целое число
     * @return  Количество символов (не учитывая 0 в конце)
     */
    uint16_t int2chars(char * buff, uint16_t a) {
        char chars[8];
        uint16_t n=0;
        while(a) {
            int x=a % 10;
            a/=10;
            char i='0';
            i=i+x;
            chars[n++]=i;
        }
        buff[0]='w';
        n++;
        for (uint16_t i = 1; i < n; i++) buff[i]=chars[n-i-1];
        buff[n]=0;
        return n;
    }



};


#endif //MWOS3_MWOSSTORAGEESPNVS_H
