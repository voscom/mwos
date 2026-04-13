#ifndef MWNETPACKET_H
#define MWNETPACKET_H
#include <Stream.h>
#include <core/MWOSDebug.h>
#ifdef ESP32
#include "mbedtls/aes.h"
#endif

// тип блока (передается в самом первом байте блока, до подписи)
enum MWBlockType: uint8_t {
    // ответ с подтверждением
    BLOCK_ANSWER = 1,
    // ответ с ошибкой
    BLOCK_ERROR = 2,
    // запрос списка контроллеров и ответ от контроллеров с их CID
    BLOCK_FIND = 3,
    // блок с данными
    BLOCK_DATA = 100,
    // блок с данными
    BLOCK_BIGDATA = 101,
    // блок с данными
    BLOCK_BIGDATA_END = 254,
};

/**
 * Работа с пакетами в буфере.
 * Формат пакета: uint8(packetType), uint16(packetID), uint16(packetSize), [данные пакета..., uint8(количество отправок), uint8(время отправки сек/10)]
 *
 * Протестировано! В заголовке плюсами отмечены протестированне методы.
 */
class MWNetPacket {
public:
    // буфер с пакетами
    uint8_t * _buffer;
    // общий размер буфера
    uint16_t _sizeBuffer=0;
    // текущий размер блоков в буфере
    uint16_t _size=0;
    // позиция текущего блока в буфере
    uint16_t _pos=0;

    /**
     * Инициализация буфера
     * @param sizeBuffer    Размер буфера, выделяемого для пакетов (от 25 байт). Если сразу не задано - надо сделать init().
     * @param buffer    Внешний буфер, если не задано - будет создан в куче
     */
    MWNetPacket(uint16_t sizeBuffer=0, uint8_t * buffer=nullptr) {
        init(sizeBuffer,buffer);
    }

    // очистить буфер (не освобождает память)
    void clear() {
        _pos=0;
        _size=0;
    }

    /**
     * Инициализация буфера
     * @param sizeBuffer    Размер буфера, выделяемого для пакетов (от 25 байт)
     * @param buffer    Внешний буфер, если не задано - будет создан в куче
     */
    void init(uint16_t sizeBuffer, uint8_t * buffer=nullptr) {
        if (_buffer || sizeBuffer<25) return;
        _sizeBuffer=sizeBuffer;
        if (buffer) _buffer=buffer;
        else _buffer=new uint8_t[sizeBuffer];
    }

    // сколько байт доступно в буфере
    uint16_t available() {
        return _sizeBuffer-_size-5; // 5 байт в конце резервируем для загрузки заголовка
    }

    /**
     * +Отправить пакет с ответом в очередь отправки
     * @param packetID
     * @param freeBuffer
     * @param packetType
     * @return Размер отправленных данных (0-не влезло, 5-отправлено)
     */
    size_t SendAnswer(uint16_t packetID, uint16_t freeBuffer, uint8_t packetType) {
        if (packetType>=BLOCK_DATA || available()<5) return 0;
        _pos=_size;
        _buffer[_size++]=packetType;
        memcpy(_buffer+_size,&packetID,2);
        _size+=2;
        memcpy(_buffer+_size,&freeBuffer,2);
        _size+=2;
#if LOG_SEND>4
        MW_LOG_TIME(); MW_LOG(F("AddAnswer ")); printPos();
#endif
        return 5;
    }

    /**
     * +Отправить пакет данных в очередь отправки.
     * @param buffer
     * @param size
     * @param packetType
     * @param packetID
     * @return Размер отправленных данных (0-не отправлено)
     */
    size_t SendPacket(void * buffer, uint16_t size, uint8_t packetType, uint16_t packetID) {
        if (size<1 || packetType<BLOCK_DATA) return 0;
        if (available()<size+5) return 0;
        _pos=_size;
        uint16_t pos=_size;
        _buffer[_size++]=packetType;
        memcpy(_buffer+_size,&packetID,2);
        _size+=2;
        //uint16_t sizeLabel=size | 0x8000; // старший бит всегда 1 (что-бы исключить метку пакета)
        memcpy(_buffer+_size,&size,2);
        _size+=2;
        memcpy(_buffer+_size,buffer,size);
        _size+=size;
        Encrypt(pos); // зашифруем этот пакет
        _buffer[_size++]=0; // количество отправок
        _buffer[_size++]=GetTimeDSec()-127; // максимально время с момента отправки
#if LOG_SEND>4
        MW_LOG_TIME(); MW_LOG(F("AddData ")); printPos();
#endif
        return size+6;
    }

    /**
     * +Установить максимальный размер для заданного пакета.
     * При необходимости разбивает заданный пакет на несколько пакетов, вставляя необходимые метаданные.
     * @param maxPacketSize Максимальный размер данных в пакете.
     * @param newPacketID Новый ID пакета. Для остальных пакетов ID идут подряд.
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return Возвращает, на сколько пакетов был разбит текущий пакет (0 - ошибка).
     */
    uint8_t SetMaxPacketSize(uint16_t maxPacketSize, uint16_t newPacketID=0, uint16_t pos=0xffff) {
        if (maxPacketSize==0) return 0;
        if (pos==0xffff) pos=_pos;
        uint16_t size=GetDataSize(pos);
        if (size<=maxPacketSize) return 1; // этот пакет не надо разбивать
        int packetsCount=size/maxPacketSize;
        if ((size % maxPacketSize)>0) packetsCount++;
        if (packetsCount>150 || _buffer[pos]!=BLOCK_DATA || packetsCount*7>available()) return 0; // пакетов слишком много или места не хватает
        uint16_t packetID=newPacketID;
        if (packetID==0) packetID=GetPacketID(pos);
        uint8_t counter=BLOCK_BIGDATA; // счетчик типа пакета
        for (int i = 0; i < packetsCount; ++i) {
            _buffer[pos]=counter;
            counter++; // каждый следующий пакет инкрементирует заголовок от BLOCK_BIGDATA (кроме последнего - BLOCK_BIGDATA_END)
            memcpy(_buffer+pos+1,&packetID,2);
            packetID++; // у следующих пакетов увеличиваем ID
            if (packetID==0) {  // нельзя 0
                packetID++;
                packetsCount++;
            }
            memcpy(_buffer+pos+3,&maxPacketSize,2);
            pos += maxPacketSize + 5; // позиция следующего пакета
            size -= maxPacketSize;
            if (maxPacketSize >= size) { // последний блок
                counter=BLOCK_BIGDATA_END;
                maxPacketSize = size;
            }
            if (i+1<packetsCount) {
                if (_size > pos) memmove(_buffer+pos+7,_buffer+pos,_size-pos); // скопируем вправо на 7 байт
                _size += 7;
            }
            _buffer[pos++]=0; // количество отправок
            _buffer[pos++]=GetTimeDSec()-127; // максимально время с момента отправки
        }
        return packetsCount;
    }

    // младший байт децисекунд (сек/10) с аптайма
    uint8_t GetTimeDSec() {
        return (uint8_t) ((millis()/100) & 0xff);
    }

    /**
     * +Установить время отправки для пакета по его позиции
     * @param skipDSec  Перенести время отправки назад на сек/10
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     */
    void SetNowTimeForPacketPos(uint8_t skipDSec=0, uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        if (pos>=_size || _buffer[pos]<BLOCK_DATA) return;
        uint16_t posNext=NextPacketPos(pos);
        _buffer[posNext-1]=GetTimeDSec()-skipDSec;  // время отправки
    }


    /**
     * +Установить время отправки для пакета по ID
     * @param packetID   ID пакета
     * @param skipDSec  Перенести время отправки назад на сек/10
     */
    void SetNowTimeForPacketID(uint16_t packetID, uint8_t skipDSec=0) {
        SetNowTimeForPacketPos(skipDSec,FindPacketPos(packetID));
    }

    /**
     * +Таймаут (сек/10) с момента последней отправки этого пакета
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return таймаут (сек/10)
     */
    uint8_t DeltaTimeDSecForPacket(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        uint16_t posNext=NextPacketPos(pos);
        int lastTimeDSec=_buffer[posNext-1];
        int deltaTime=(int) GetTimeDSec() - lastTimeDSec;
        if (deltaTime>=0) return (uint8_t) deltaTime;
        return (uint8_t) (255+deltaTime);
    }

    /**
     * +Количество попыток этого пакета
     * @param incrimentCount Увеличить количество попыток (+1)
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @param newValueCount  Новое значение для записи в счетчик (-1 - нет)
     * @return Количество попыток (отправок) этого пакета
     */
    uint8_t PacketCount(bool incrimentCount=true, uint16_t pos=0xffff, int16_t newValueCount=-1) {
        if (pos==0xffff) pos=_pos;
        uint16_t posNext=NextPacketPos(pos);
        if (incrimentCount) _buffer[posNext-2]++;
        if (newValueCount>=0) _buffer[posNext-2]=newValueCount;
        return _buffer[posNext-2];
    }

    /**
     * +Получить размер пакета по его позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return  Размер пакета в этой позиции
     */
    uint16_t GetPacketSize(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        if (_buffer[pos]<BLOCK_DATA) return 5;
        return _buffer[pos+3] + (_buffer[pos+4] << 8) + 7;
    }

    /**
     * +Получить размер данных  по позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return  Размер данных в этой позиции
     */
    uint16_t GetDataSize(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        return _buffer[pos+3] + (_buffer[pos+4] << 8);
    }

    /**
     * Получить ссылку на массив данных  по позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return  Массив данных в этой позиции
     */
    uint8_t * GetDataBuffer(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        return _buffer + pos + 5;
    }

    /**
     * +Получить ID пакета по его позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return  ID пакета в этой позиции
     */
    uint16_t GetPacketID(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        return _buffer[pos+1] + (_buffer[pos+2] << 8);
    }

    /**
     * +Получить тип пакета по его позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return  Тип пакета в этой позиции
     */
    uint8_t GetPacketType(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        return _buffer[pos];
    }

    /**
     * +Найти следующий пакет, за текущей позицией
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return Позиция следующего пакета
     */
    uint16_t NextPacketPos(uint16_t pos=0xffff) {
        bool currentPos=(pos==0xffff);
        if (currentPos) pos=_pos;
        if (pos>=_size) return _size;
        pos+=GetPacketSize(pos);
        if (pos>=_size) return _size;
        if (currentPos) _pos=pos;
        return pos;
    }

    /**
     * +Найти позицию пакета с ID
     * @param packetID  ID пакета
     * @return Позиция пакета с этим ID (равно _size, если не найден пакет)
     */
    uint16_t FindPacketPos(uint16_t packetID) {
        uint16_t pos=0;
        while (pos<_size) {
            if (GetPacketID(pos)==packetID) return pos;
            pos=NextPacketPos(pos);
        }
        return pos;
    }

    /**
     * Установить на первый неотправленный пакет в очереди.
     * Но сначала выбирает ответы и ошибки.
     * @return  Успешно нашли пакет
     */
    bool FindUnsendPacketPos(uint8_t resendTimeoutDSec) {
        uint16_t pos=0;
        while (pos<_size) { // ищем подтверждения или ошибки
            if (_buffer[pos]<BLOCK_DATA) { // это пакет подтверждения или ошибки
                _pos=pos;
                return true;
            }
            pos=NextPacketPos(pos);
        }
        pos=0;
        while (pos<_size) { // ищем первый неотправленный пакет
            if (PacketCount(false,pos)==0 || DeltaTimeDSecForPacket(pos)>resendTimeoutDSec) { // пакет не отправлен или отправлен давно
                _pos=pos;
                return true;
            }
            pos=NextPacketPos(pos);
        }
        return false;
    }

    /**
     * +Удалить пакет в текущей позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return Пакет успешно удален
     */
    bool RemovePacketPos(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        if (pos>=_size) return false;
        uint16_t nextP=NextPacketPos(pos);
        if (nextP<=pos) return false;
        if (nextP<_size) memcpy(_buffer+pos,_buffer+nextP,_size-nextP); // сдвинем то, что за пакетом
        _size-=nextP-pos;
        return true;
    }

    /**
     * +Удалить пакет по ID
     * @param packetID
     * @return
     */
    bool RemovePacketID(uint16_t packetID) {
        return RemovePacketPos(FindPacketPos(packetID));
    }

    /**
     * +Вставить свободное место для нового пакета из конца очереди.
     * Сначала в конец очереди дописывается заголовок пакета.
     * Потом для этого заголовка ищет место в очереди и вставляет туда пакет из конца очереди.
     * @param posTo     Куда вставить пакет
     * @param posFrom   Место в конце, откуда вставить пакет
     * @return Пакет успешно вставлен
     */
    bool InsertPacketPos(uint16_t posTo, uint16_t posFrom=0xffff) {
        if (posFrom==0xffff) posFrom=_pos;
        uint16_t size=GetPacketSize(posFrom);
        if (size<5 || available()<size) return false;
        if (posTo>=posFrom) return true; // вставлять не надо
        uint8_t buffFrom[5];
        memcpy(&buffFrom, _buffer+posFrom, 5); // запомним заголовок
        memmove(_buffer+posTo+size,_buffer+posTo,_size-posTo); // скопируем вправо
        // скопируем заголовок
        memcpy(_buffer+posTo, &buffFrom, 5);
        return true;
    }

    /**
     * +Дешифрует пакет в текущей позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return Пакет успешно дешифрован
     */
    bool Decrypt(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        if (pos>=_size) return false;
        MW_LOG(F("Decrypt: ")); MW_LOG_LN(pos);
        aes_ctr_crypt(_buffer+pos+5,GetDataSize(pos), GetPacketID(pos));
        return true;
    }

    /**
     * +Шифрует пакет в текущей позиции
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return Пакет успешно зашифрован
     */
    bool Encrypt(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        if (pos>=_size) return false;
        MW_LOG(F("Encrypt: ")); MW_LOG_LN(pos);
        aes_ctr_crypt(_buffer+pos+5,GetDataSize(pos), GetPacketID(pos));
        return true;
    }

    /**
     * +Реализация шифрования AES CTR 128.
     * Только для ESP32! Поддерживает аппаратное ускорение шифрования.
     * @param data
     * @param len
     * @param blockId
     */
    void aes_ctr_crypt(uint8_t* data, uint16_t len, uint16_t blockId) {
#ifdef MWOS_CRYPT_KEY
        const uint8_t key[16] = MWOS_CRYPT_KEY;
        uint64_t userHash=MWOS_USER_HASH;
        uint8_t nonce[16];
        memcpy(nonce, &userHash, 8);
        memset(nonce + 8, 0, 6);
        memcpy(nonce + 14, &blockId, 2);
        uint8_t stream_block[16];
        size_t nc_off = 0;

#ifdef ESP32
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_enc(&aes, key, 128);
        mbedtls_aes_crypt_ctr(&aes, len, &nc_off, nonce, stream_block, data, data);
        mbedtls_aes_free(&aes);
#endif
#endif
    }

    void printDumpPos(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        uint16_t blockSize=GetPacketSize(pos)-2;
        MW_LOG(F(" dumpNet: ")); MW_LOG_BYTES(_buffer+pos,blockSize,blockSize);
        MW_LOG_LN();
    }

    void printPos(uint16_t pos=0xffff) {
        if (pos==0xffff) pos=_pos;
        uint16_t blockSize=GetPacketSize(pos);
        if (blockSize==5) {
            MW_LOG(F(" netPacket pos: ")); MW_LOG(pos);
            MW_LOG(F(", type: ")); MW_LOG(GetPacketType(pos));
            MW_LOG(F(", ID: ")); MW_LOG(GetPacketID(pos));
            MW_LOG(F(", freeBuff: ")); MW_LOG_LN(GetDataSize(pos));
            return;
        }
        if (pos>=_size) {
            MW_LOG_LN(F(" Out size!"));
            return;
        }
        MW_LOG(F(" netPacket pos: ")); MW_LOG(pos);
        MW_LOG(F(", type: ")); MW_LOG(GetPacketType(pos));
        MW_LOG(F(", ID: ")); MW_LOG(GetPacketID(pos));
        blockSize -= 7;
        MW_LOG(F(", size: ")); MW_LOG(blockSize);
        MW_LOG(F(", count: ")); MW_LOG(PacketCount(false,pos));
        MW_LOG(F(", time: ")); MW_LOG(DeltaTimeDSecForPacket(pos));
        MW_LOG(F(", dump: ")); MW_LOG_BYTES(_buffer+pos+5,blockSize,blockSize);
        MW_LOG_LN();
    }

    void printAll() {
        MW_LOG_LN();
        MW_LOG(F("===== netPacket list: ")); MW_LOG_LN(_size);
        int pos=0;
        while (pos<_size) {
            printPos(pos);
            pos=NextPacketPos(pos);
        }
        MW_LOG_LN();
        MW_LOG_FLUSH();
    }



};


#endif
