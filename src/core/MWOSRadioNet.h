#ifndef MWOS3_MWOSRadioNet_H
#define MWOS3_MWOSRadioNet_H

#include <condition_variable>

#include "core/MWOSModule.h"
#include "core/net/MWBus.h"
#include "core/adlib/MWTimeout.h"
#include "core/net/RadioPacket.h"

/**
 * Модуль радиосети - беспроводная связь.
 * Передает и принимает данные по радиосети. С ретрансляцией.
 * При приеме нового пакета вызывает onEvent(EVENT_RECEIVE_RADIO_PACKET)
 * Для отправки пакета - sendPacket
 *
 */
class MWOSRadioNet : public MWOSModule {
public:

    enum StatusRadioNet: uint8_t {
        RN_NOT_INIT,
        RN_LOAD_OPTIONS,
        RN_SAVE_OPTIONS,
        RN_RECEIVE_OPTIONS,
        RN_WAIT_RECEIVE_DATA,
        RN_RECEIVE_DATA,
        RN_WAIT_SEND_DATA,
        RN_SEND_DATA,
    };

    Stream * _stream;
    // пакеты для отправки (однонаправленный массив)
    RadioPacket radioPacket;
    // получаемый сейчас пакет
    RadioPacket radioPacketReceive;
    // [0] - количество отправляемых пакетов, на которые не получено подтверждение, [1] - количество отправленных запросов, [2] - количество полученных ответов
    uint32_t _sendErrorPackets[3] = {0,0,0};

#pragma pack(push,1)
    // текущий статус радиосети
    StatusRadioNet _status=RN_NOT_INIT;
    // Максимальное количество повторов передачи
    uint8_t _maxCountSend=10;
    // Максимальное количество повторов ретрансляции
    uint8_t _maxCountReSend=3;
    // статус индикатора приема - получен ответный пакет до отправки запроса второй раз (сбрасывается при повторной отправке пакета)
    uint8_t _indicatorReceive=0;
    // статус индикатора ошибки
    uint8_t _indicatorError=0;
    // максимальный размер пакета для радиосети
    uint16_t _maxPacketSize=240;
    // гарантированное время приема после передачи (и не менее 0.5-1.9сек от свободного Aux)
    uint16_t _msecReceive=2000;
    // время пинга [сек,сек] {сразу, через час аптайма} (если 0, то без пингов)
    uint16_t _pingSecund[2]={5,1800};

    // сколько пакетов принято в этом радиофрейме
    uint16_t _receivePacketsInRadioFrame=0;
    // номер текущего радиопакета (id пакета)
    uint16_t _radioPacketID=0;
    // время секунд до перезагрузки контроллера с момента получения последнего пакета (0-нет)
    uint16_t _restartTimeoutSec=4000;

    uint16_t _radioIDs[MWOS_RADIONET_SLOTS];
#pragma pack(pop)

    // текущий статус радиосети
    MWOS_PARAM(0, status, mwos_param_uint8, mwos_param_option+mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // номер текущего радиопакета (id пакета)
    MWOS_PARAM(1, radioPacketID, mwos_param_uint16, mwos_param_option+mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // Максимальное количество повторов передачи
    MWOS_PARAM(2, maxCountSend, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // Максимальное количество повторов ретрансляции
    MWOS_PARAM(3, maxCountReSend, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // id доступных вокруг устройств (определяется автоматом)
    MWOS_PARAM(4, radioIDs, mwos_param_uint16, mwos_param_option+mwos_param_readonly, MWOS_STORAGE_NO, MWOS_RADIONET_SLOTS);
    // [0] - количество отправляемых пакетов, на которые не получено подтверждение, [1] - количество отправленных запросов, [2] - количество полученных ответов
    MWOS_PARAM(5, sendErrorPackets, mwos_param_uint32, mwos_param_option+mwos_param_readonly, MWOS_STORAGE_NO, 3);
    // время пинга [сек,сек] (если 0, то без пингов)
    MWOS_PARAM(6, pingSecund, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 2);
    // гарантированное время приема после передачи (и не менее 0.5-1.9сек от свободного Aux)
    MWOS_PARAM(7, msecReceive, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // время секунд до перезагрузки контроллера с момента получения последнего пакета (0-нет)
    MWOS_PARAM(8, restartTimeoutSec, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    /**
     * Создать модуль. Для каждого модуля необходим основной конструктор без аргументов!
     * конструкторы с аргументами - можно задавать дополнительно.
     * тут необходимо добавить все созданные параметры.
     * и можно переопределить имя и тип модуля.
     */
    MWOSRadioNet() : MWOSModule((char *) F("radioNet")) {
        moduleType=MODULE_RADIONET;
        // добавим параметры модуля
        AddParam(&p_status);
        AddParam(&p_radioPacketID);
        AddParam(&p_maxCountSend);
        AddParam(&p_maxCountReSend);
        AddParam(&p_radioIDs);
        AddParam(&p_sendErrorPackets);
        AddParam(&p_pingSecund);
        AddParam(&p_msecReceive);
        AddParam(&p_restartTimeoutSec);
        for (uint8_t i=0; i<MWOS_RADIONET_SLOTS; i++) _radioIDs[i]=0;
    }

    void deviceInit(Stream * bus, uint16_t maxPacketSize) {
        _maxPacketSize=maxPacketSize;
        _stream=bus;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // только при инициализации системы
            _maxCountSend= loadValue(_maxCountSend, &p_maxCountSend, 0);
            _maxCountReSend= loadValue(_maxCountReSend, &p_maxCountReSend, 0);
            _pingSecund[0]= loadValue(_pingSecund[0], &p_pingSecund, 0);
            _pingSecund[1]= loadValue(_pingSecund[1], &p_pingSecund, 1);
            _msecReceive= loadValue(_msecReceive, &p_msecReceive, 0);
            _restartTimeoutSec= loadValue(_restartTimeoutSec, &p_restartTimeoutSec, 0);
            _restartTimeout.start(_restartTimeoutSec*10);
            //_indicatorError=0;
            //_indicatorSend=0;
            _indicatorReceive=0;
            radioPacketReceive.setReceivePacket();
        } else
        if (modeEvent==EVENT_UPDATE) {
            if (_restartTimeoutSec>_pingSecund[2] && _restartTimeout.isTimeout() && _restartTimeout.isStarted()) {
                ESP.restart(); // перезагрузим контроллер, если не получали пакетов радиосети более _restartTimeoutSec секунд
            }
            switch (_status) {
                case RN_WAIT_RECEIVE_DATA: waitAIR(); break;
                case RN_WAIT_SEND_DATA: waitAIR(); break;
                case RN_RECEIVE_DATA: receivePackets(); break;
                case RN_SEND_DATA: sendPackets(); break;
            }
        }
        MWOSModule::onEvent(modeEvent);
    }

    /**
      * Вызывается при запросе значения параметра
      * Читает значение из хранилища (кроме байтовых массивов - из надо читать вручную)
      * @param paramNum  Номер параметра
      * @param arrayIndex Номер индекса в массиве значений параметра (если это массив)
      * @return  Значение
      */
    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 0: return _status;
            case 1: return _radioPacketID;
            case 2: return _maxCountSend;
            case 3: return _maxCountReSend;
            case 4: return _radioIDs[arrayIndex];
            case 5: return _sendErrorPackets[arrayIndex];
            case 6: return _pingSecund[arrayIndex];
            case 7: return _msecReceive;
            case 8: return _restartTimeoutSec;
        }
        return MWOSModule::getValue(param, arrayIndex);
    }

    /**
     * Добавить пакет для отправки
     * @param buffer        Отправляемый буфер (только данные без заголовка)
     * @param packetSize    Размер пакета данных (до 240 максимум)
     * @param toNID         Кому (0-всем, но без ретрансляции)
     */
    void sendPacket(uint8_t * buffer, uint8_t packetSize, uint16_t toNID) {
        _radioPacketID++;
        SetParamChanged(&p_radioPacketID);
        RadioPacket * sendPacket=radioPacket.Add(buffer,packetSize,RadioPacket::RPT_REQ,toNID,getNID(),_radioPacketID);
#if (LOG_RADIO>2)
        MW_LOG_TIME(); MW_LOG(F("send")); sendPacket->printPacketToLog();
#endif
    }

    /**
     * @return  модуль свободен и готов к приему новых команд или данных
     */
    virtual bool IsAux() {
        return _timeout.isTimeout();
    }

protected:
    MWTimeout _timeout;
    MWTimeout _sendTimeout;
    // таймаут от момента получения последнего пакета (до перезагрузки контроллера)
    MWTimeout _restartTimeout;
    // таймаут от последнего момента, когда радиосеть была занята
    MWTimeout _receiveFreeAirTimeout;
    uint16_t nowSendPacketSize=0;
    uint16_t randomTimeoutFromFreeAir=0;
    // размер буфера отправки на момент начала операции
    uint16_t _streamAvailableForWrite=0;

    void setStatus(StatusRadioNet newStatus) {
        if (newStatus==_status) return;
        _status=newStatus;
        _timeout.startMS(20);
        switch (_status) {
            case RN_RECEIVE_DATA: {
                _timeout.startMS(_msecReceive);
                _receivePacketsInRadioFrame=0;
                randomTimeoutFromFreeAir=random(500,1900);
                _receiveFreeAirTimeout.startMS(randomTimeoutFromFreeAir);
            } break;
            case RN_SEND_DATA: {
                nowSendPacketSize=_maxPacketSize;
                _timeout.startMS(50);
            } break;
        }
        SetParamChanged(&p_status);
#if (LOG_RADIO>3)
        MW_LOG_MODULE(this); MW_LOG(F("setStatus: ")); MW_LOG_LN((uint16_t) newStatus);
#endif

    }

    // перед переключением между режимом приема и передачи - дожидаемся освобождения радиоканала
    void waitAIR() {
        if (!IsAux()) return; // дожидаемся выставления уровня на M0 и M1
        while (_stream->available()) _stream->read(); // прочитаем все байты из приемного буфера, что-бы его очистить
        setStatus((StatusRadioNet) (((int) _status)+1));
        if (_streamAvailableForWrite!=_stream->availableForWrite()) {
            _streamAvailableForWrite=_stream->availableForWrite();
#if (LOG_RADIO>3)
            MW_LOG_MODULE(this); MW_LOG(F("WriteBuff: ")); MW_LOG_LN(_streamAvailableForWrite);
#endif
        }
    }

    uint8_t addNID(uint16_t newNID) {
        for (uint8_t i=0; i<MWOS_RADIONET_SLOTS; i++) {
            if (_radioIDs[i]==0 && newNID>0) {
                _radioIDs[i]=newNID;
                SetParamChanged(&p_radioIDs,i);
#if (LOG_RADIO>1)
                MW_LOG_MODULE(this); MW_LOG(i); MW_LOG(F(" = new NID ")); MW_LOG_LN(newNID);
#endif
            }
            if (_radioIDs[i]==newNID) return i;
        }
        return 0;
    }

    // получить радиосетевой ID этого контроллера
    uint16_t getNID() {
#if CID>0
        return (uint16_t) CID;
#else
        return (uint16_t) MWOSNetModule::getNetModule()->getValueByParamId(0,0);
#endif
    }

    // количество пакетов для отправки
    uint16_t countSendPackets() {
        uint16_t res=0;
        RadioPacket * nextRadioPacket=&radioPacket;
        while (nextRadioPacket!=NULL) {
            if (nextRadioPacket->packet.packetType!=RadioPacket::RPT_EMPTY) res++;
            nextRadioPacket=nextRadioPacket->_nextRadioPacket;
        }
        return res;
    }

    // Отправить все пакеты из очереди в стрим
    void sendPackets() {
        uint16_t sendPacketsCount=sendPackets(RadioPacket::RPT_ANSWER); // сначала отправим ответы - у них приоритет больше
        sendPacketsCount+=sendPackets(RadioPacket::RPT_CMD); // потом команды
        sendPacketsCount+=sendPackets(RadioPacket::RPT_REQ); // потом запросы
        if (sendPacketsCount==0 && _streamAvailableForWrite <= _stream->availableForWrite()) { // буфер отправки полностью свободен
            // значит все уже отправлено
            _stream->flush(); // убедимся, что закончили отправку по UART (может отправляется последний байт)
            setStatus(RN_WAIT_RECEIVE_DATA); // и теперь можно переключаться на прием, когда освободит AUX
        }
    }

    /**
     * Отправить пакеты из очереди в стрим
     * @param radioPacketType   Тип отправляемых пакетов
     * @return Количество отправленных пакетов
     */
    uint16_t sendPackets(RadioPacket::RadioPacketType radioPacketType) {
        uint16_t res=0;
        uint16_t nid=getNID();
        RadioPacket * nextRadioPacket=&radioPacket;
        RadioPacket * prevRadioPacket=NULL;
        while (nextRadioPacket!=NULL) {
            uint16_t maxCountSend=_maxCountSend;
            if (nextRadioPacket->packet.fromNID!=nid // пакет не наш - это ретрансляция
                || nextRadioPacket->_buffer==NULL) maxCountSend=_maxCountReSend; // или служебный пакет - повторяем как для ретрансляции
            uint16_t lastSendPacketSize=nowSendPacketSize;
            nowSendPacketSize-=nextRadioPacket->Send(_stream,radioPacketType,nowSendPacketSize,maxCountSend,prevRadioPacket);
            if (nextRadioPacket->IsError()) { // ошибка - переполнено количество отправок пакета
                //_indicatorError = 1; // признак ошибки
                _sendErrorPackets[0]++;
                SetParamChanged(&p_sendErrorPackets,0);
#if (LOG_RADIO>0)
                MW_LOG_MODULE(this); MW_LOG(F("sendErrorPackets=")); MW_LOG_LN(_sendErrorPackets[0]);
#endif
            } else if (lastSendPacketSize>nowSendPacketSize) { // пакет успешно отправлен
                if (millis()>3600000) // если более часа аптайм, то пинг [1]
                    _sendTimeout.start(_pingSecund[1]*10); // через час аптайма
                else // меньше часа аптайм - пинг [0]
                    _sendTimeout.start(_pingSecund[0]*10); // до часа аптайма
                res++;
                //_indicatorSend = 1; // признак отправки
                if (nextRadioPacket->sendCount>1) _indicatorReceive=0; // это не первая отправка пакета - сбросим индикатор приема
                if (nextRadioPacket->packet.packetType==RadioPacket::RadioPacketType::RPT_REQ || nextRadioPacket->packet.packetType==RadioPacket::RadioPacketType::RPT_CMD) {
                    _sendErrorPackets[1]++;
                    SetParamChanged(&p_sendErrorPackets,1);
#if (LOG_RADIO>1)
                    MW_LOG_MODULE(this); MW_LOG(F("sendReqPackets=")); MW_LOG_LN(_sendErrorPackets[1]);
#endif
                }
            }
            prevRadioPacket=nextRadioPacket;
            nextRadioPacket=nextRadioPacket->_nextRadioPacket;
        }
        return res;
    }

    void receivePackets() {
        if (!IsAux() || _stream->available()>0) { // радиосеть занята
            _receiveFreeAirTimeout.startMS(randomTimeoutFromFreeAir); // свободный эфир
        }
        //_indicatorSend = 0; // признак отправки
        if (radioPacketReceive.ReceivePacket(_stream)) { // получен очередной пакет данных
            onReceivePacket();
        }
        if (_timeout.isTimeout() && _receiveFreeAirTimeout.isTimeout()) { // безусловно ждем на приеме msecReceive и свободного эфира
            if (countSendPackets()==0) {
                if (_pingSecund[1]>0 && _sendTimeout.isTimeout()) ping();
                else {
                    _timeout.startMS(_msecReceive);
                    return;
                }
            }
            setStatus(RN_WAIT_SEND_DATA);
        }
    }

    void ping() {
#if (LOG_RADIO>2)
        MW_LOG_MODULE(this); MW_LOG_LN(F("Ping"));
#endif
        sendPacket(NULL,0,0);
    }

    // получен очередной пакет в radioPacketReceive
    void onReceivePacket() {
        addNID(radioPacketReceive.packet.fromNID); // добавим к списку сетевых ID
#if (LOG_RADIO>3)
        MW_LOG_MODULE(this); MW_LOG(F("onReceive")); radioPacketReceive.printPacketToLog();
#endif
        _restartTimeout.start(_restartTimeoutSec*10);
        if (radioPacketReceive.packet.packetType==RadioPacket::RPT_ANSWER) { // принят ответ
            RadioPacket * nextRadioPacket=&radioPacket;
            while (nextRadioPacket!=NULL) {
                if (nextRadioPacket->packet.fromNID==radioPacketReceive.packet.toNID && nextRadioPacket->packet.id==radioPacketReceive.packet.id) {
                    nextRadioPacket->Free(RadioPacket::RFM_ReceivePacketOk); // освободим подтвержденный пакет
                }
                nextRadioPacket=nextRadioPacket->_nextRadioPacket;
            }
            _sendErrorPackets[2]++;
            SetParamChanged(&p_sendErrorPackets,2);
#if (LOG_RADIO>1)
            MW_LOG_MODULE(this); MW_LOG(F("receiveAnswerPackets=")); MW_LOG_LN(_sendErrorPackets[2]);
#endif
        }
        uint16_t nid=getNID();
        if (radioPacketReceive.packet.toNID==nid || radioPacketReceive.packet.toNID==0) { // это наш пакет
            if (radioPacketReceive.packet.packetType==RadioPacket::RPT_REQ) { // это запрос
                // вызовем обработку полученного запроса
                mwos.dispatchEvent(EVENT_RECEIVE_RADIO_PACKET,this);
                // отправим ответ
                radioPacket.Add(NULL,0,RadioPacket::RPT_ANSWER,radioPacketReceive.packet.fromNID,nid, radioPacketReceive.packet.id);
            }
            if (radioPacketReceive.packet.packetType==RadioPacket::RPT_CMD && radioPacketReceive.packet.size>17) { // это команда
                // обработаем receive_args
                startCommand();
            }
        } else
            radioPacket.Add(&radioPacketReceive); // не наш пакет - добавим на ретрансляцию
        // освободим принятый пакет после обработки
        _receivePacketsInRadioFrame++;
        _indicatorReceive = 1; // индикатор приема
        //_indicatorError = 0; // сбросим индикатор ошибки
        radioPacketReceive.Free(RadioPacket::RFM_OnReceivePacket);
    }

    void startCommand() {
        MWOSNetReceiverFields receive_args;
        receive_args.cmd=(MWOSProtocolCommand) radioPacketReceive._buffer[0];
        if (receive_args.cmd < 128) { // это команда контроллеру (без аргументов)
            receive_args.module_id=radioPacketReceive._buffer[2] << 8 | radioPacketReceive._buffer[1];
            if (receive_args.cmd < 64) { // это команда модулю (без аргументов)
                receive_args.param_id=radioPacketReceive._buffer[4] << 8 | radioPacketReceive._buffer[3];
                if (receive_args.cmd < 32) { // это команда параметру (без аргументов)
                    receive_args.array_index=radioPacketReceive._buffer[6] << 8 | radioPacketReceive._buffer[5];
                    if (receive_args.cmd < 16) { // это команда параметру с индексом массива (без аргументов)
                        uint16_t receiveBlockSize=radioPacketReceive._buffer[8] << 8 | radioPacketReceive._buffer[7];
                        if (receiveBlockSize>0 && receiveBlockSize<=radioPacketReceive.packet.size-17) {
                            receive_args.Start(receiveBlockSize); // займем память
                            memcpy(receive_args.buffer, radioPacketReceive._buffer+9, receiveBlockSize);
                            //for (uint16_t i=0; i<receiveBlockSize; i++) receive_args.buffer[receive_args.size]=radioPacketReceive._buffer[9+i];
                        }
                    }
                }
            }
        }
        switch (receive_args.cmd) {
            case mwos_server_cmd_param_set_value: {
                // не сохранять по месту хранения
                MWOSModule * receiveModule=(MWOSModule *) mwos.FindChildById(receive_args.module_id);
                if (receiveModule!=NULL) {
                    receiveModule->onReceiveValue(&receive_args);
                    MWOSParam * receiveParam=(MWOSParam *) receiveModule->FindChildById(receive_args.param_id);
                    if (receiveParam!=NULL) {
                        receiveModule->SetParamChanged(receiveParam, receive_args.array_index, true);
                    }
                }
            } break;
            case mwos_server_cmd_param_get_value: {
                //sendPacket();
                // запросили значение параметра
                //receiveModule->SetParamChanged(receiveParam, receive_args.array_index, true);
            } break;
        }
        // отправим ответ
        radioPacket.Add(NULL,0,RadioPacket::RPT_ANSWER,radioPacketReceive.packet.fromNID,getNID(), radioPacketReceive.packet.id);
    }


};
#endif
