#ifndef MWNETPACKETRECEIVER_H
#define MWNETPACKETRECEIVER_H

#include <Stream.h>

#include "MWNetPacketTransmitter.h"
#include "core/adlib/MWTimeout.h"
#include "core/adlib/LibCRC.h"

#ifndef ReceiveTimeoutDSec
// время (сек/10) таймаута приема пакета
#define ReceiveTimeoutDSec 20
#endif

#ifndef NetPacketBlocks
// сколько id последних блоков запоминать, что-бы не принимать повторно
#define NetPacketBlocks 20
#endif

#ifndef MWOS_MAX_READ_PER_FRAME
#define MWOS_MAX_READ_PER_FRAME 1024	// чтение максимум байт за один фрейм (не более INT32_MAX)
#endif

/**
 * Принимает пакеты из потока, извлекает из него фреймы данных, и отправляет их адресату.
 * Необходимо настроить буфер приема и передачи receivePacket.init и  !
 *
 * Если фрейм данных длиннее пакета:
 * 1. Получает несколько пакетов
 * 2. При необходимости, запрашивает недостающие или сбойные пакеты (отправляет ошибку).
 * 3. Сортирует полученные пакеты.
 * 4. Извлекает фрейм из нескольких пакетов.
 * 5. Отправляет их адресату.
 *
 * - При получении каждого пакета данных отправляет подтверждение (или ошибку).
 * - При получении подтверждения - удаляет пакет данных из списка отправки.
 * - При получении ошибки - повторно отправляет пакет данных из списка отправки.
 *
 */
class MWNetPacketReceiver : public MWNetPacketTransmitter {
protected:
    // шаг приема блока
    uint8_t _receiveStep=0;
    // тип принимаемого блока
    MWBlockType _receiveBlockCmd;
    // позиция принимаемого байта в последнем блоке
    uint16_t _receiveBytePos=0;
    // полученная контрольная сумма блока
    uint16_t _receiveBlockCRC16;
    // текущий блок в
    uint16_t lastBlockIDsNum=0;
    // список ID последних принятых блоков
    uint16_t lastBlockIDs[NetPacketBlocks];
    // контрольная сумма приема
    uint16_t receive_crc16;
    // последний доступный размер в буфере приема
    uint16_t lastAvailable=0;

public:
    //  время (сек/10) таймаута приема пакета
    uint8_t receiveTimeoutDSec=ReceiveTimeoutDSec;
    // время (сек/10), через которое повторно обрабатывать принятые блоки, не обработанные с первого раза
    uint8_t reReceiveTimeoutDSec=30;

    // буфер для приема
    MWNetPacket receivePacket;
    // время от получения последнего пакета
    MWTimeout<uint16_t,1000> lastReceiveTimeout;

    /**
     * Инициализация. Можно задать сразу или позже _stream и sendPacket.init(). Если задать буфер (или только размер), то он будет пополам поделен на прием и передачу.
     * @param stream        Ввод/вывод
     * @param sizeBuffer    Суммарный размер буфера приема и передачи. Обязательно - четный, что-бы ровно поделить пополам.
     * @param buffer        Внешний буфер приема и передачи, если не задан - будет создан в куче
     */
    MWNetPacketReceiver(void * stream=nullptr, uint16_t sizeBuffer=0, uint8_t * buffer=nullptr) : MWNetPacketTransmitter(stream,sizeBuffer/2,buffer,sizeBuffer/2) {
        if (sizeBuffer>=50) receivePacket.init(sizeBuffer/2, buffer);
        _pauseReceive=false;
        newSession();
    }

    // начать новую сессию приема (обычно после переподключения)
    void newSession() {
        for (int i = 0; i < NetPacketBlocks; ++i) lastBlockIDs[i]=0;
        lastBlockIDsNum=0;
        receivePacket.clear();
        sendPacket.clear();
    }

    /**
     * Для наследования обработки событий
     * @param eventType Тип события
     * @return  Событие обработано успешно. После успешного NetPacketReceive блок будет удален из буфера приема и отправлено подтверждение.
     */
    virtual bool onNetEvent(NetPacketEvent eventType) {
        return true;
    }

    /**
     * Вызывается каждый фрейм ОС
     * @return  Был получен новый пакет
     */
    bool updateRead() {
        if (!_stream) return false;
        if (_stream->available()>0) {
            // приоритет на получение данных - прием работает одинаково для любого типа сетей
            int16_t maxData=MWOS_MAX_READ_PER_FRAME; // за такт до килобайта
            while (_stream->available()>0 && maxData-- > 0) {
                if (readNexByte(_stream->read())) return true;
            }
        } else {
            if (lastReceiveTimeout.isTimeout(100) // давно ничего не приходило
                && (lastAvailable >> 1) > receivePacket.available()) // и приемный буфер освободился более чем вдвое
                AddAnswer(0,BLOCK_ANSWER); // отправим размер приемного буфера
        }
        updateWrite();
        if (_receiveStep==0 && receivePacket._size>0) CheckReceiveBlocks(); // когда не идет прием и там что-то есть
        return false;
    }

    /**
     * Проверить все входящие блоки и обработать их повторно
     */
    void CheckReceiveBlocks() {
        // у входящих блоков тоже есть таймауты и количество попыток
        uint16_t pos=0;
        bool needCheckBigBlock=false;
        while (pos<receivePacket._size) { // пробежим все принятые пакеты
            if (receivePacket.GetPacketType(pos)==BLOCK_DATA && receivePacket.DeltaTimeDSecForPacket(pos)>reReceiveTimeoutDSec) {
                receivePacket._pos=pos;
                if (onNetEvent(NetPacketReceive)) {
                    uint16_t blockID=receivePacket.GetPacketID();
                    receivePacket.RemovePacketPos();
#if LOG_RECEIVE>4
                    MW_LOG_TIME(); MW_LOG(F("Remove answer, available: ")); MW_LOG_LN(receivePacket.available());
#endif
                    AddAnswer(blockID, BLOCK_ANSWER); // отправим подтверждение приема
                } else {
                    receivePacket.SetNowTimeForPacketPos(); // запомним время последней обработки блока
                    receivePacket.PacketCount(); // увеличим количество попыток
                }
                return;
            }
            if (receivePacket.GetPacketType(pos)>BLOCK_DATA) {
                receivePacket.SetNowTimeForPacketPos(pos); // начнем отсчет заново
                //receivePacket.PacketCount(true,pos);
                needCheckBigBlock=true;
            }
            pos=receivePacket.NextPacketPos(pos);
        }
        if (needCheckBigBlock) CheckBigBlock();
    }

    /**
     * +Добавить пакет с ответом в очередь отправки
     * @param packetID
     * @param packetType
     * @return Размер отправленных данных (0-не влезло, 5-отправлено)
    */
    size_t AddAnswer(uint16_t packetID, uint8_t packetType) {
        lastAvailable=receivePacket.available();
        return sendPacket.SendAnswer(packetID,lastAvailable,packetType);
    }

    /**
     * убрать символы экранирования
     * @param b Код символа
     * @return  Это нормальный символ (false - экранирован)
     */
    bool readEscaped(uint8_t &b) {
        if (b == 0x7D)   { _escapedRead = true;  return false; }
        if (_escapedRead) {
            _escapedRead = false; // Сбрасываем всегда, когда пришел байт ПОСЛЕ ESC
            if (b == 0x5E) { b = MWOS_BEGIN_PACKET_MARKER; return true; }
            if (b == 0x5D) { b = 0x7D;   return true; }
            return false; // Если после 0x7D пришло что-то не то — это мусор, игнорируем
        }
        return true;
    }

    /**
     * Обрабатывает очередной байт из стрима, на предмет команды для контроллера
     * Все передачи данных для любых контроллеров всегда начинаются с подписи MWOS_BEGIN_PACKET_MARKER
     * затем идет двухбайтовая длина блока, а потом уже все остальное
     * Если обнаружена подпись и считана длина, то _cmdMode == 5
     * @param receiveByte   Очередной полученный байт
     * @return  Был получен новый пакет
     */
    bool readNexByte(uint8_t receiveByte) {
        //MW_LOG('~'); MW_LOG_BYTE(receiveByte);
        if (receiveByte==MWOS_BEGIN_PACKET_MARKER) { // это пришел маркер начала пакета
            if (_receiveStep>0) receiveClear(99); // предыдущий пакет не до конца
            receive_crc16 = crc16_start();
            receivePacket._pos=receivePacket._size;
            _escapedRead=false;
            _receiveStep=1;
            _receiveBytePos=0;
            return false;
        }
        if (!readEscaped(receiveByte)) return false; // это байт экранирования
        switch (_receiveStep) {
            case 1: // тип пакета
                WriteToBuffer(receiveByte);
                _receiveBlockCmd=(MWBlockType) receiveByte;
                _receiveStep++;
                return false;
            case 2: // младший байт id пакета
                WriteToBuffer(receiveByte);
                _receiveStep++;
                return false;
            case 3: // старший байт id пакета
                WriteToBuffer(receiveByte);
                _receiveStep++;
                return false;
            case 4: // младший байт длины
                WriteToBuffer(receiveByte);
                _receiveStep++;
                return false;
            case 5: {
                // старший байт длины
                WriteToBuffer(receiveByte);
#if (LOG_RECEIVE>4)
                //MW_LOG_TIME(); MW_LOG(F("Receive start ")); MW_LOG_BYTES(receivePacket._buffer+receivePacket._pos,5,5); MW_LOG_LN();
#endif
                if (_receiveBlockCmd < BLOCK_DATA) {
                    _receiveStep=7; // если это не прием данных - пропустим прием пакета - сразу принимаем CRC
                } else { // к приему пакета
                    uint16_t blockID=receivePacket.GetPacketID();
                    uint16_t blockSize=receivePacket.GetDataSize();
                    if (blockSize>receivePacket.available()) { // это данные и для них нет места
                        AddAnswer(blockID, BLOCK_ERROR); // отправим ошибку приема
#if (LOG_RECEIVE>1)
                        MW_LOG_TIME(); MW_LOG(F("Big size: ")); MW_LOG(blockSize); receivePacket.printPos();
#endif
                        receiveClear(_receiveStep);
                        return false;
                    }
                    // найдем место, куда вставлять блок
                    uint16_t posPacket=receivePacket.FindPacketPos(blockID);
                    if (posPacket<receivePacket._size && receivePacket.GetPacketID(posPacket)==blockID) { // такой блок уже получили - проигнорируем его
                        AddAnswer(blockID, BLOCK_ANSWER); // отправим подтверждение приема (на всякий случай)
#if (LOG_RECEIVE>1)
                        MW_LOG_TIME(); MW_LOG(F("Double ID: ")); receivePacket.printPos(posPacket);
#endif
                        receiveClear(_receiveStep);
                        return false;
                    }
                    // вставим блок
                    receivePacket._size+=receivePacket.GetPacketSize();
                    receivePacket.InsertPacketPos(posPacket); // вставим блок в позицию по ID
                    _receiveStep++;
                }
            } return false;
            case 6: // прием пакета
                if (WriteToBuffer(receiveByte)) _receiveStep++; // пакет принят полностью
                return false;
            case 7: // младший байт CRC16
                _receiveBlockCRC16=receiveByte;
                _receiveStep++;
                return false;
            case 8: // старший байт CRC16
                _receiveBlockCRC16|=receiveByte << 8;
                _receiveStep=0; // приняли пакет
                receiveClear(0);
                if (_receiveBlockCRC16 != receive_crc16) {
                    if (_receiveBlockCmd >= BLOCK_DATA) {
                        AddAnswer(receivePacket.GetPacketID(), BLOCK_ERROR); // это данные - отправим ошибку приема
                        receivePacket._size=receivePacket._pos; // удалим принимаемый блок
                    }
#if (LOG_RECEIVE>0)
                    MW_LOG_TIME(); MW_LOG(F("Receive CRC Error ")); MW_LOG(_receiveBlockCRC16,HEX); MW_LOG('='); MW_LOG_LN(receive_crc16,HEX);
#endif
                } else {
                    lastReceiveTimeout.start();
                    return CheckReceiveBlock();
                }
        }
        return false;
    }

    /**
     * Записать байт в буфер приема
     * @param b Байт
     * @return  Был записан последний байт - пора прекратить
     */
    bool WriteToBuffer(uint8_t b) {
        receive_crc16 = addCRC16(receive_crc16, b);
        receivePacket._buffer[receivePacket._pos+_receiveBytePos]=b;
        _receiveBytePos++;
        return receivePacket.GetDataSize()+5<=_receiveBytePos;
    }

    /**
     * Обработать принятый блок данных
     */
    bool CheckReceiveBlock() {
        bool res=false;
        uint16_t blockID=receivePacket.GetPacketID();
        if (_receiveBlockCmd == BLOCK_ANSWER) {
            _freeBuffer=receivePacket.GetDataSize();
#if (LOG_RECEIVE>3)
            MW_LOG_TIME(); MW_LOG(F("Receive answer ID: ")); MW_LOG(blockID); MW_LOG(F(", freeBuff= ")); MW_LOG_LN(_freeBuffer);
#endif
            sendPacket.RemovePacketPos(); // это пришло подтверждение - удалим пакет из очереди отправки
#if LOG_SEND>4
            MW_LOG_TIME(); MW_LOG(F("Remove ID: ")); MW_LOG(blockID); MW_LOG(F(", available: ")); MW_LOG_LN(sendPacket.available());
#endif
        }
        else if (_receiveBlockCmd == BLOCK_ERROR) {
            _freeBuffer=receivePacket.GetDataSize();
#if (LOG_RECEIVE>3)
            MW_LOG_TIME(); MW_LOG(F("Receive error ID: ")); MW_LOG(blockID); MW_LOG(F(", freeBuff= ")); MW_LOG_LN(_freeBuffer);
#endif
            sendPacket.SetNowTimeForPacketID(blockID,resendTimeoutDSec+1); // это пришла ошибка - сразу повторно отправим блок (сразу увеличим его таймаут)
        }
        else { // теперь проверим, что это за блок пришел
#if (LOG_RECEIVE>3)
            MW_LOG_TIME(); MW_LOG(F("Receive block: ")); receivePacket.printPos();
#endif
            if (_receiveBlockCmd >= BLOCK_DATA && CheckBlockId(blockID)) { // этот блок уже принимали раньше
                AddAnswer(blockID, BLOCK_ANSWER); // отправим подтверждение приема (на всякий случай)
                receivePacket.RemovePacketPos(); // удалим этот блок - он не актуален
#if LOG_RECEIVE>4
                MW_LOG_TIME(); MW_LOG(F("Remove Double, available: ")); MW_LOG_LN(receivePacket.available());
#endif
                return false;
            }
            receivePacket.Decrypt(); // любой блок сначала расшифруем
            if (_receiveBlockCmd == BLOCK_DATA) {
                if (onNetEvent(NetPacketReceive)) { // обработаем полученный блок (и очистим от него место, если все нормально)
                    uint16_t blockID=receivePacket.GetPacketID();
                    receivePacket.RemovePacketPos();
#if LOG_RECEIVE>4
                    MW_LOG_TIME(); MW_LOG(F("Remove after event, available: ")); MW_LOG_LN(receivePacket.available());
#endif
                    AddAnswer(blockID, BLOCK_ANSWER); // отправим подтверждение приема
                } else {
                    receivePacket.SetNowTimeForPacketPos(); // запомним время последней обработки блока
                    receivePacket.PacketCount(); // увеличим количество попыток
                }
                return true;
            } else {
                if (_receiveBlockCmd > BLOCK_BIGDATA) // полько если не первый пакет
                    res=CheckBigBlock(); // принимаем большой блок данных - он сам может вызвать onEvent
                AddAnswer(blockID, BLOCK_ANSWER); // отправим подтверждение приема
            }
        }
        return res;
    }

    // Этот блок уже принимали? Если нет, то запомним его в списке, как принятый.
    bool CheckBlockId(uint16_t blockID) {
        if (blockID == 0) return false;
        for (int i = 0; i < NetPacketBlocks; ++i) {
            if (lastBlockIDs[i]==blockID) return true;
        }
        lastBlockIDsNum++;
        if (lastBlockIDsNum>=NetPacketBlocks) lastBlockIDsNum=0;
        lastBlockIDs[lastBlockIDsNum]=blockID;
        return false;
    }

    /**
     * +Проверить прогресс получения большого блока (из нескольких пакетов).
     * При необходимости запрашивает недостающие пакеты.
     * За один вызов выделяет не больше одного большого блока.
     */
    bool CheckBigBlock() {
        int32_t firstPacketPos=-1;
        int32_t lastPacketId=-1;
        int32_t lastPacketType=BLOCK_DATA;
        uint16_t pos=0;
        while (pos<receivePacket._size) { // пробежим все принятые пакеты
            uint16_t packetType=receivePacket.GetPacketType(pos);
            if (packetType>=BLOCK_BIGDATA) {
                uint16_t packetId=receivePacket.GetPacketID(pos);
                if (packetType==BLOCK_BIGDATA) { // это первый
                    firstPacketPos=pos;
                } else
                    if (packetType==BLOCK_BIGDATA_END) { // это последний пакет
                        if (lastPacketId<0)
                            SendErrorForPackets(packetId-1, 1); // не было предыдущих пакетов - запросим один
                        else
                            if (packetId>lastPacketId+1) // пропущены пакеты по id
                                SendErrorForPackets(packetId-1, packetId - lastPacketId - 1);
                            else { // все пакеты найдены - отправим большой блок по назначению
                                CompleteBigBlock(firstPacketPos,pos);
                                return true;
                            }
                    } else { // это не последний пакет
                        if (packetType>lastPacketType+1) // пропущены пакеты по типу
                            SendErrorForPackets(packetId-1, packetType - lastPacketType - 1);

                    }
                lastPacketType=packetType;
                lastPacketId=packetId;
            }
            pos=receivePacket.NextPacketPos(pos);
        }
        return false;
    }

    /**
     * +Создает из цепочки маленьких пакетов BLOCK_BIGDATA один большой пакет BLOCK_DATA.
     * Вызывает onEvent для объединенного пакета.
     * @param firstPacketPos    Первый пакет в цепочке
     * @param endPacketPos      Последний пакет в цепочке
     */
    void CompleteBigBlock(uint16_t firstPacketPos,uint16_t endPacketPos) {
#if (LOG_RECEIVE>1)
        MW_LOG(F("CompleteBigBlock: ")); MW_LOG(receivePacket.GetPacketID(firstPacketPos)); MW_LOG(F(" - ")); MW_LOG_LN(receivePacket.GetPacketID(endPacketPos));
#endif
        receivePacket._buffer[firstPacketPos]=BLOCK_DATA;
        receivePacket._buffer[firstPacketPos+1]=0;
        receivePacket._buffer[firstPacketPos+2]=0; // id = 0
        receivePacket._pos=firstPacketPos;
        uint16_t newDataSize=receivePacket.GetDataSize(firstPacketPos);
        uint16_t pos=receivePacket.NextPacketPos(firstPacketPos);
        while (pos<=endPacketPos) { // пробежим все пакеты
            newDataSize+=receivePacket.GetDataSize(pos);
            uint16_t posNext=receivePacket.NextPacketPos(pos);
            // вырежем 7 служебных байт
            memcpy(receivePacket._buffer+pos-2,receivePacket._buffer+pos+5,receivePacket._size-pos-5);
            receivePacket._size-=7;
            endPacketPos-=7;
            pos=posNext-7;
        }
        receivePacket._buffer[firstPacketPos+3] = newDataSize & 0xff;
        receivePacket._buffer[firstPacketPos+4] = (newDataSize >> 8) & 0xff;
        receivePacket.PacketCount(false,firstPacketPos,0); // количество попыток
        if (onNetEvent(NetPacketReceive)) { // обработаем полученный блок (и очистим от него место, если все нормально)
            receivePacket.RemovePacketPos();
#if LOG_RECEIVE>4
            MW_LOG_TIME(); MW_LOG(F("Remove after big event, available: ")); MW_LOG_LN(receivePacket.available());
#endif
        } else {
            receivePacket.SetNowTimeForPacketPos(); // запомним время последней обработки блока
            receivePacket.PacketCount(); // увеличим количество попыток
        }
    }

    /**
     * Отправить запрос на несколько пакетов
     * @param toBlockID До какого id пакета включительно
     * @param countBlocks Сколько пакетов
     */
    void SendErrorForPackets(uint16_t toBlockID, uint8_t countBlocks) {
        for (uint16_t i = toBlockID-countBlocks+1; i <= toBlockID; ++i) {
            uint16_t pos=receivePacket.FindPacketPos(i);
            if (receivePacket.DeltaTimeDSecForPacket(pos)>reReceiveTimeoutDSec) {
#if (LOG_RECEIVE>2)
                MW_LOG_TIME(); MW_LOG(F("SendError block ID: ")); MW_LOG_LN(i);
#endif
                // запросим недостающие в очереди пакеты
                AddAnswer(i, BLOCK_ERROR);
            }
        }
    }

    void receiveClear(uint8_t code) {
        if (code>0) {
#if (LOG_RECEIVE>0)
            MW_LOG_TIME(); MW_LOG(F("receiveClear: ")); MW_LOG(_receiveStep); MW_LOG(';'); MW_LOG_LN(code);
#endif
        }
        _receiveStep=0;
    }


};



#endif //MWNETPACKETRECEIVER_H
