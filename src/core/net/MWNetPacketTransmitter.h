#ifndef MWNETPACKETTRANSMITTER_H
#define MWNETPACKETTRANSMITTER_H
#include <Stream.h>
#include <core/adlib/LibCRC.h>
#include <core/adlib/MWTimeout.h>

#include "MWNetPacket.h"

#ifndef MaxSendCount
// максимальное количество повторных отправок блока (до 127)
#define MaxSendCount 10
#endif

#ifndef ResendTimeoutDSec
// время (сек/10), через которое повторно отправлять пакеты
#define ResendTimeoutDSec 50
#endif

#ifndef ReserveForSendAnswer
// сколько байт зарезервировать для SendAnswer
#define ReserveForSendAnswer 15
#endif

// маркер начала пакета связи
#define MWOS_BEGIN_PACKET_MARKER 0x7F

/**
 * Отправляет пакеты в поток.
 *
 * Сначала добавляет пакеты в очередь.
 * Потом, постепенно передает пакеты из очереди (по мере загрузки сети).
 * При получении подтверждения - пакет удаляется из очереди.
 * При получении ошибки - пакет повторно отправляется из очереди.
 * Для неподтвержденных пакетов - повторно отправляет их через таймаут ResendTimeoutDSec и повторяет отправку MaxSendCount раз
 *
 * При отправке слишком длинного блока - автоматически разбивает его на несколько пакетов.
 *
 * Пакет отправляется в поток побайтно, что-бы влезть даже в маленькие аппаратные фреймы.
 *
 * Если не хватает места на получателе, то приостанавливает отправку SendPacket
 *
 */
class MWNetPacketTransmitter {
public:
    // последний отправленный пакет
    uint16_t _lastPacketID=0;
    // максимальный размер пакета. Если данные больше, то режутся на такие пакеты.
    uint16_t maxPacketSize=1024;
    // размер свободного места на получателе (из пакетов BLOCK_ANSWER и BLOCK_ERROR)
    uint16_t _freeBuffer=0xffff;

    // контрольная сумма отправки
    uint16_t _crc16Send;
    // размер текущего блока для отправки
    uint16_t _sendBlockSize=0;
    // текущий байт побайтовой асинхронной отправки
    uint16_t _sendOffset=0;

    MWTimeout<uint16_t,1000> sendTimeout;

    /*********/
    // текущий шаг побайтовой асинхронной отправки
    int8_t _sendStep:4;
    // поставить на паузу передачу
    uint8_t _pauseSend:1;
    // поставить на паузу прием
    uint8_t _pauseReceive:1;
    // сеть в режиме AP сервера
    uint8_t IsServerSetup:1;
    // сеть отключена
    uint8_t IsNetOff:1;
    /*********/
    // тип сети
    MWOSNetType net_module_type:5;
    // при отправке блока произошла ошибка
    uint8_t writeError:1;
    // пропущено при чтении
    uint8_t _escapedRead:1;
    // резерв
    uint8_t :1;
    /*********/
    // время (сек/10), через которое повторно отправлять пакеты
    uint8_t resendTimeoutDSec=ResendTimeoutDSec;
    // максимальное количество повторных отправок блока
    uint8_t maxSendCount=0;

    // буфер отправки
    MWNetPacket sendPacket;
    // для отправки данных
    Stream * _stream=nullptr;

    /**
    *  Инициализация. Можно задать сразу или позже _stream и sendPacket.init()
     * @param stream        Ввод/вывод
     * @param sizeBuffer    Размер буфера, выделяемого для пакетов (от 25 байт). Если сразу не задано - надо сделать init().
     * @param buffer        Внешний буфер, если не задано - будет создан в куче
     * @param offset        Смещение во внешнем буфере
     */
    MWNetPacketTransmitter(void * stream, uint16_t sizeBuffer=0, uint8_t * buffer=nullptr,uint16_t offset=0) {
        _stream=(Stream *) stream;
        resendTimeoutDSec=ResendTimeoutDSec;
        _pauseSend=0;
        _sendStep=0;
        if (sizeBuffer>=25) {
            if (buffer) buffer += offset;
            sendPacket.init(sizeBuffer, buffer);
        }
    }

    /**
     * Для наследования обработки событий
     * @param eventType Тип события
     * @return  Событие обработано успешно.
     */
    virtual bool onNetEvent(NetPacketEvent eventType) {
        return true;
    }

    // +вызывается каждый фрейм ОС
    void updateWrite() {
        if (!_pauseSend && sendPacket._size>0) {
            //if (netType) { // HalfDuplex - отправляем сразу блок
                if (!IsBusy()) SendAll(); // отправим, если еще не отправляли
            //} else { // FullDuplex - отправляем, пока не отправим один блок, или не упремся
            //    while (_stream->availableForWrite()>0 && AsyncSendFirstUnsendPacket()) {};
            //}
        }
    }

    // получить следующий доступный ID пакета
    uint16_t getNextPacketID() {
        _lastPacketID++;
        if (_lastPacketID==0) _lastPacketID++; // нельзя 0
        return _lastPacketID;
    }

    /**
     * +Добавить пакет данных в очередь отправки.
     * Если буфер отправки больше максимального пакета - порежем на пакеты.
     * Если не лезет в очередь отправки или другая ошибка - возвращает 0.
     * Резервирует ReserveForSendAnswer байт в буфере отправки для SendAnswer
     * @param buffer
     * @param size
     * @return Размер отправленных данных (0-не отправлено)
     */
    size_t AddPacket(void * buffer, uint16_t size) {
        uint8_t packetType=((uint8_t*) buffer)[0];
        if (size<1 || packetType<BLOCK_DATA) return 0;
        uint16_t realSize=0;
        uint16_t sendOffset=0;
        if (size>maxPacketSize) {
            int packetsCount=size/maxPacketSize;
            if ((size % maxPacketSize)>0) packetsCount++;
            if (sendPacket.available()<size+7*packetsCount+ReserveForSendAnswer) return 0;
            if (packetsCount>250) return 0;
            int packetNum=0;
            while (size>maxPacketSize) { // отправим полные блоки
                packetType=BLOCK_BIGDATA+packetNum; // первый блок тип = BLOCK_BIGDATA
                packetNum++;
                if (packetNum>=packetsCount) packetType=BLOCK_BIGDATA_END; // это признак последнего пакета
                size_t sizeSended=sendPacket.SendPacket((uint8_t*)buffer + sendOffset,maxPacketSize,packetType,getNextPacketID());
                if (sizeSended==0) return realSize;
                realSize+=sizeSended;
                sendOffset+=maxPacketSize;
                size-=maxPacketSize;
                if (size==0) return realSize; // уложились в тютельку - нечего досылать
            }
            // дошлем жопку с size<maxPacketSize
            packetType=BLOCK_BIGDATA_END; // это признак последнего пакета
        }
        if (sendPacket.available()<size+7+ReserveForSendAnswer) return 0;
        realSize+=sendPacket.SendPacket((uint8_t*)buffer + sendOffset,size,packetType,getNextPacketID());
        return realSize;
    }

    /**
     * +Установить максимальный размер для заданного пакета в буфере отправки.
     * При необходимости разбивает заданный пакет на несколько пакетов, вставляя необходимые метаданные.
     * @param pos   Позиция пакета в буфере (если не задано - текущая позиция)
     * @return Возвращает, на сколько пакетов был разбит текущий пакет (0 - ошибка).
     */
    int8_t SetSendMaxPacketSize(uint16_t pos=0xffff) {
        int8_t res = sendPacket.SetMaxPacketSize(maxPacketSize,_lastPacketID,pos);
        _lastPacketID += res;
        return res;
    }

    /**
     * Отправить подряд несколько блоков из очереди отправки, пока не заполним размер maxPacketSize + 10.
     * Удобно использовать, когда есть переключение на прием/передачу. После отправки можно вычислить время (по размеру отправленного) и установить таймер.
     * @return Сколько байт отправили
     */
    uint16_t SendAll() {
        if (!sendTimeout.isTimeout()) return 0;
        _sendBlockSize=0;
        while (sendPacket.FindUnsendPacketPos(resendTimeoutDSec)) {
            sendPacket.SetNowTimeForPacketPos(); // учтем время отправки даже если потом не отправим
            uint16_t sendBytes=SendPacketNow();
            if (sendBytes==0) {
                sendTimeout.start(1);
                return _sendBlockSize;
            }
#if LOG_SEND>4
            MW_LOG_TIME(); MW_LOG(F("SendAll: ")); MW_LOG(_sendBlockSize);  MW_LOG(F(", available: ")); MW_LOG_LN(sendPacket.available());
#endif
            _sendBlockSize += sendBytes;
        }
        sendTimeout.start(1);
        return _sendBlockSize;
    }

     // Модуль занят приемом или передачей
    virtual bool IsBusy() {
        return false; // _timeout.isTimeout();
    }

    /**
     * Отправить текущий пакет. Удобно использовать, когда есть переключение на прием/передачу
     * @return Отправленный размер
     */
    uint16_t SendPacketNow() {
        uint16_t sizeP=sendPacket.GetPacketSize();
        if (sendPacket._buffer[sendPacket._pos]>=BLOCK_DATA) sizeP -= 2; // без 2х служебных байт в конце
        if (_sendBlockSize+sizeP > maxPacketSize+5 || sizeP + 3 > _stream->availableForWrite()) {
            return 0;
        }
        int32_t sizeSend=beginBlock(); // подпись
        uint16_t crc16Send = calcCRC16_buffer(sendPacket._buffer+sendPacket._pos,sizeP);
        for (int i = 0; i < sizeP; ++i) {
            sizeSend+=write(sendPacket._buffer[sendPacket._pos+i]);
        }
        sizeSend+=write((uint8_t) (crc16Send & 0xff));
        sizeSend+=write((uint8_t) ((crc16Send >> 8) & 0xff));
        sizeP +=3; // учтем заголовок (1b) и RCR16
        if (sizeSend < sizeP) {
#if LOG_SEND>0
            MW_LOG_TIME(); MW_LOG(F("SendPacketNow error: ")); MW_LOG(sizeSend); MW_LOG('<'); MW_LOG_LN(sizeP);
#endif
            return 0;
        }
#if LOG_SEND>4
        MW_LOG_TIME(); MW_LOG(F("SendPacketNow: ")); MW_LOG(sizeSend); sendPacket.printPos();
#endif
        if (sendPacket.GetPacketType()>=BLOCK_DATA) {
            if (sendPacket.PacketCount(true)>maxSendCount) {
                sendPacket.RemovePacketPos(); // блок превысил количество отправок - удалим его из очереди
#if LOG_SEND>4
                MW_LOG_TIME(); MW_LOG(F("Remove max, available: ")); MW_LOG_LN(sendPacket.available());
#endif
            }
        } else {
            sendPacket.RemovePacketPos(); // если это ответный блок - удалим его из очереди
#if LOG_SEND>4
            MW_LOG_TIME(); MW_LOG(F("Remove anws, available: ")); MW_LOG_LN(sendPacket.available());
#endif
        }
        return sizeSend;
    }

    /**
      * +Отправить пакет побайтно, что-бы лезло даже в маленький буфер отправки.
      * Удобно использовать, когда нет переключения на прием/передачу
      * @return Идет отправка
      */
    bool AsyncSendFirstUnsendPacket() {
        bool res=false;
        switch (_sendStep) {
            case 0: {
                if (_stream==nullptr || sendPacket._size==0) return false;
                if (!sendPacket.FindUnsendPacketPos(resendTimeoutDSec)) return false; // найдем первый неотправленный блок
                sendPacket.SetNowTimeForPacketPos(); // учтем время отправки даже если потом не отправим
                _sendOffset=sendPacket._pos;
                _sendBlockSize=sendPacket.GetPacketSize();
                if (sendPacket.GetPacketType()>=BLOCK_DATA) {
                    _sendBlockSize-=2; // последние 2 байта данных не отправляем (это количество и время отправки)
                    if (_freeBuffer<_sendBlockSize+5) {
#if LOG_SEND>0
                        MW_LOG_TIME(); MW_LOG(F("Target space out: ")); MW_LOG(_freeBuffer); sendPacket.printPos();
#endif
                        return false; // на получателе нет места
                    }
                }
#if LOG_SEND>4
                MW_LOG_TIME(); MW_LOG(F("SendPacket ")); sendPacket.printPos();
#endif
                _sendStep++;
                return true;
            } break;
            case 1: res=beginBlock(); _crc16Send = crc16_start(); if (res) _sendStep++; break;
            case 2: {
                res=write(sendPacket._buffer[_sendOffset],true)>0;
                if (res) {
                    _sendOffset++;
                    if (_sendOffset >= sendPacket._pos+_sendBlockSize) _sendStep++;
                }
            } break;
            case 3: res=(write((uint8_t) (_crc16Send & 0xff),false)>0); if (res) _sendStep++; break;
            case 4: res=(write((uint8_t) ((_crc16Send >> 8) & 0xff),false)>0); if (res) _sendStep++; break;
            case 5: {
#if LOG_SEND>4
                MW_LOG_TIME(); MW_LOG(F("Send OK ")); sendPacket.printPos();
#endif
                if (sendPacket.GetPacketType()>=BLOCK_DATA) {
                    if (sendPacket.PacketCount(true)>maxSendCount) {
                        sendPacket.RemovePacketPos(); // блок превысил количество отправок - удалим его из очереди
#if LOG_SEND>4
                        MW_LOG_TIME(); MW_LOG(F("Remove maxCount, available: ")); MW_LOG_LN(sendPacket.available());
#endif
                    }
                } else {
                    sendPacket.RemovePacketPos(); // если это ответный блок - удалим его из очереди
#if LOG_SEND>4
                    MW_LOG_TIME(); MW_LOG(F("Remove answer, available: ")); MW_LOG_LN(sendPacket.available());
#endif
                }
                _sendStep=0; // признак окончания отправки
            } break;
            default: _sendStep=0;
        }
        return res;
    }

    size_t beginBlock() {
        size_t size=_stream->write(MWOS_BEGIN_PACKET_MARKER);
        writeError=size<1;
        return size;
    }

    size_t write(uint8_t b, bool crc=false) {
        if (writeError) return 0; //  после ошибки не отправляем
        if (sendPacket._buffer[sendPacket._pos]>=BLOCK_DATA) { // при отправке данных - учитываем место на получателе
            _freeBuffer--;
            if (_freeBuffer<2) {
                writeError=true;
                return 0;
            }
        }
        // экранируем маркер
        const uint8_t ESC = 0x7D;
        size_t size=0;
        if (b == MWOS_BEGIN_PACKET_MARKER) { // MWOS_BEGIN_PACKET_MARKER = 0x7F
            if (_stream->availableForWrite()<2) return 0;
            size+=_stream->write(ESC);
            size+=_stream->write(0x5E); // Заменяем 0x7F на 0x7D 0x5E
            writeError=size<2;
            if (!writeError) size=1;
        } else if (b == ESC) {
            if (_stream->availableForWrite()<2) return 0;
            size+=_stream->write(ESC);
            size+=_stream->write(0x5D); // Заменяем 0x7D на 0x7D 0x5D
            writeError=size<2;
            if (!writeError) size=1;
        } else {
            size+=_stream->write(b);
            writeError=size<1;
        }
        if (crc && !writeError) _crc16Send = addCRC16(_crc16Send, b);
        return size;
    }

};



#endif //MWNETPACKETTRANSMITTER_H
