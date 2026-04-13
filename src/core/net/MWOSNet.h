#ifndef MWOS3_MWOSNet_H
#define MWOS3_MWOSNet_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include <core/net/MWNetPacketReceiver.h>
#include "core/net/MWNetFrame.h"

#ifndef MWOSNET
#define MWOSNET 1
#endif

#ifndef CIDS_LIST_SIZE
// сколько может быть контроллеров в подчиненной сети
#define CIDS_LIST_SIZE 5
#endif

/**
 * Модуль net
 *
 *
 */
class MWOSNet : public MWOSModule, public MWNetPacketReceiver {
public:

    // найти основной сетевой модуль
    static MWOSNet * getNetModule() {
        return (MWOSNet *) mwos.FindChildByModuleType(MODULE_NET);
    }

    // код контроллера
    MWOS_PARAM(0, controllerId, PARAM_UINT32, PARAM_TYPE_READONLY+PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);
    // текущее состояние сети: Выключено; Включено; Включена точка доступа и сервер настроек
    MWOS_PARAM_FF(1, netStatus, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1, "Выключено;Включено;Точка доступа");
    // при первой инициализации: Нет (ничего не трогать); Включать сеть; Выключать сеть; Включать точку доступа и сервер настроек
    MWOS_PARAM_FF(2, netInit, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1, "Нет;Выключено;Включено;Точка доступа");
    // номер текущего пакета (id пакета)
    MWOS_PARAM(3, packetID, PARAM_UINT16, PARAM_TYPE_OPTIONS+PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // Максимальное количество повторов передачи (если не было подтверждения)
    MWOS_PARAM(4, maxCountSend, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // время (сек/10), через которое повторно отправлять пакеты (если не было подтверждения)
    MWOS_PARAM(5, resendTimeoutDSec, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // id доступных устройств
    MWOS_PARAM(6, listCIDs, PARAM_UINT16, PARAM_TYPE_OPTIONS+PARAM_TYPE_READONLY, MWOS_STORAGE_NO, CIDS_LIST_SIZE);
    // время пинга [сек,сек] (если 0, то без пингов)
    MWOS_PARAM(7, pingSecund, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 2);
    // время секунд до перезагрузки контроллера с момента получения последнего пакета (0-нет)
    MWOS_PARAM(8, restartTimeoutSec, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // тип сети - FullDuplex или HalfDuplex (жестко задается из модуля устройства)
    MWOS_PARAM_FF(9, netType, PARAM_BITS5, PARAM_TYPE_OPTIONS+PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 3, MWOSNetTypeNames);

    MWNetFrame frame;

    // список доступных в сети контроллеров (дополняется автоматически, когда от контроллеров приходят данные)
    uint32_t _listCIDs[CIDS_LIST_SIZE];
    MWTimeout<uint16_t,1> _restartTimeout;
    // таймаут соединения (1/15 сек)
    MWTimeout<uint16_t,15,true> _connect_timeout;
    // время пинга [сек,сек] {сразу, через час аптайма} (если 0, то без пингов)
    uint16_t _pingSecund[2]={5,1800};
    // время секунд до перезагрузки контроллера с момента получения последнего пакета (0-нет)
    // должно быть минимум на 10 сек больше времени PING
    uint16_t _restartTimeoutSec=0;
    /***********/
    // текущий шаг соединения
    MWOSNetStep connectedStep:4;
    // текущий статус соединения
    ConnectStatus connectStatus:4;
    /***********/
    // счетчик переподключений к серверу до переподключение к сети (7 попыток) или выбора другого сервера (4 попытки, если включено)
    uint8_t reconnectServerStep:3;
    // резерв
    uint8_t :5;
    /***********/

    /**
     * Инициализация сетевого модуля. Можно задать сразу или позже _stream и sendPacket.init(). Если задать буфер (или только размер), то он будет пополам поделен на прием и передачу.
     * @param stream        Ввод/вывод
     * @param sizeBuffer    Суммарный размер буфера приема и передачи. Обязательно - четный, что-бы ровно поделить пополам.
     * @param buffer        Внешний буфер приема и передачи, если не задан - будет создан в куче
     */
    MWOSNet(void * stream=nullptr, uint16_t sizeBuffer=0, uint8_t * buffer=nullptr) : MWOSModule((char *) F("net")), MWNetPacketReceiver(stream,sizeBuffer,buffer)  {
        moduleType=ModuleType::MODULE_NET;
        net_module_type=Net_unknow;
        connectedStep=STEP_NET_CLOSE;
        reconnectServerStep=0;
        for (uint8_t i=0; i<CIDS_LIST_SIZE; i++) _listCIDs[i]=0;
        frame._net=this;
#if defined(MWOS_RESET_TO_FACTORY_SETTINGS_PIN) && (MWOS_RESET_TO_FACTORY_SETTINGS_PIN>=0)
        pinMode(MWOS_RESET_TO_FACTORY_SETTINGS_PIN,INPUT_PULLUP); // ножка с кнопкой сброса - настроим на вход с подтяжкой на +
        delay(100);
        if (digitalRead(MWOS_RESET_TO_FACTORY_SETTINGS_PIN)==LOW) { // ножка с кнопкой сброса посажена на +
            MW_LOG_LN(F("RESET_TO_FACTORY_SETTINGS!"));
            MWEEPROM.clear(); // очистим EEPROM
            connectedStep=STEP_SERVER_AP;
        } else
#endif
    }

    void deviceInit(Stream * stream, uint16_t packetSize) {
        maxPacketSize=packetSize;
        _stream=stream;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
        switch (modeEvent) {
            case EVENT_UPDATE: {
                if (connectedStep==STEP_NET_OFF) return;
                if (_restartTimeoutSec>_pingSecund[1]+10 && _restartTimeout.isTimeout() && _restartTimeout.isStarted()) {
                    resetController(); // перезагрузим контроллер, если не получали пакетов сети более _restartTimeoutSec секунд
                }
                if (_connect_timeout.isTimeout()) NetStep();
                if (canRead()) updateRead();
                if (canSend()) frame.update();
            } break;
            case EVENT_INIT: {
                maxSendCount= loadValueInt(maxSendCount, p_maxCountSend, 0);
                resendTimeoutDSec= loadValueInt(resendTimeoutDSec, p_resendTimeoutDSec, 0);
                _pingSecund[0]= loadValueInt(_pingSecund[0], p_pingSecund, 0);
                _pingSecund[1]= loadValueInt(_pingSecund[1], p_pingSecund, 1);
                _restartTimeoutSec= loadValueInt(_restartTimeoutSec, p_restartTimeoutSec, 0);
                uint8_t st=loadValueInt((MWOSNET > 0),p_netInit,0);
                MW_LOG_MODULE(this); MW_LOG(F("LoadStatus = ")); MW_LOG_LN(st);
                SetStatus((NetStatus) st);
                randomSeed(micros()); // переставим генератор случайных чисел
            } break;
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_controllerId: mwos.saveCID(data.toInt()); setConnectedStep(STEP_SERVER_CONNECTED); break;
                    case id_maxCountSend: maxSendCount= data.toInt(); break;
                    case id_resendTimeoutDSec: resendTimeoutDSec= data.toInt(); break;
                    case id_pingSecund: _pingSecund[data.param_index]= data.toInt(); break;
                    case id_restartTimeoutSec: _restartTimeoutSec= data.toInt(); _restartTimeout.start(_restartTimeoutSec); break;
                    case id_netStatus: SetStatus((NetStatus) data.toInt()); break;
                    default: ;
                }
            } break;
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    // выдадим сразу из переменной
                    case id_controllerId: data.setValueInt(mwos.getCID()); return;
                    case id_packetID: data.setValueInt(_lastPacketID); return;
                    case id_maxCountSend: data.setValueInt(maxSendCount); return;
                    case id_resendTimeoutDSec: data.setValueInt(resendTimeoutDSec); return;
                    case id_listCIDs: return data.setValueInt(_listCIDs[data.param_index]); return;
                    case id_pingSecund: data.setValueInt(_pingSecund[data.param_index]); return;
                    case id_restartTimeoutSec: data.setValueInt(_restartTimeoutSec); return;
                    case id_netType: data.setValueInt(net_module_type); return;
                    case id_netStatus: data.setValueInt(GetStatus()); return;
                    default: ;
                }
            } break;
            case EVENT_POWER_OFF: {
                SetStatus(NS_netOff);
            } break;
        }
        MWOSModule::onEvent(modeEvent,data);
    }

    /**
     * Перевести сеть в новое состояние
     * @param newStatus Новое состояние сети Отключено/Подключено/AP
     */
    void SetStatus(NetStatus newStatus) {
        IsNetOff=(newStatus==NS_netOff);
        IsServerSetup=(newStatus==NS_netAP);
#if (LOG_NET>2)
        MW_LOG_MODULE(this); MW_LOG(F("NetStatus = ")); MW_LOG_LN(newStatus);
#endif
        onConnectChange(DISCONNECT_COMMAND);
    }

    /**
     * @return Текущий статус сети
     */
    NetStatus GetStatus() {
        if (IsNetOff) return NS_netOff;
        if (IsServerSetup) return NS_netAP;
        return NS_netActive;
    }

    /**
     * @return Текущий статус соединения
     */
    ConnectStatus GetConnectStatus() {
        return connectStatus;
    }

    /**
     * Можно переопределить, что-бы считывать реальный статус подключения к серверу или сети.
     * @param toServer К серверу (false - к сети)
     * @return  Текущий статус подключения к сети
     */
    virtual bool isConnected(bool toServer) {
        if (toServer) return (connectedStep > STEP_SERVER_CONNECT && connectedStep < STEP_NET_OFF);
        return (connectedStep > STEP_NET_START && connectedStep < STEP_NET_OFF);
    }

    // можно отправлять данные?
    bool canSend() {
        return (connectedStep >= STEP_SERVER_CONNECTED && connectedStep <= STEP_SERVER_CONNECTED);
    }

    // можно принимать данные?
    bool canRead() {
        return (connectedStep >= STEP_SERVER_WAIT_ID && connectedStep <= STEP_SERVER_CONNECTED);
    }

    /**
     * Включить сеть
     */
    void start() {
        SetStatus(NS_netActive);
    }

    /**
     * Включить режим AP-сервера
     */
    void startAP() {
        SetStatus(NS_netAP);
    }

    /**
     * Отключить сеть
     */
    void stop() {
        SetStatus(NS_netOff);
    }

protected:

    // начать новую сессию связи
    void startNewSession() {
        newSession();
        frame.startSession=false;
    }

    /**
     * Вызывается после подключения и отключения. Можно переопределить.
     * @param connectCode   Код причины подключения/отключения
     */
    virtual void onConnectChange(MWOSConnectCode connectCode) {
#if (LOG_NET>1)
        MW_LOG_MODULE(this); MW_LOG(F("onConnectChange: ")); MW_LOG_LN(connectCode);
#endif
        if (connectCode>=CONNECT_COMMAND) { // это подключение - Connect
            mwos.SetChangedAll(2,false); // повторно пошлем все параметры, у кого не снят флаг sended
            MWValue data=MWValue(PARAM_AUTO,id,0xffff,0xffff); // передаем только id модуля
            mwos.dispatchEvent(EVENT_CONNECT,data);
        } else { // это отключение - Disconnect
            if (connectedStep == STEP_SERVER_CONNECTED) {
                MWValue data=MWValue(PARAM_AUTO,id,0xffff,0xffff); // передаем только id модуля
                mwos.dispatchEvent(EVENT_DISCONNECT,data);
            }
            if (connectedStep>=STEP_NET_CONNECT && connectCode!=DISCONNECT_CONNECT_SERVER_COUNT && isConnected(false)) {
                setConnectedStep(STEP_NET_CONNECT); // перейдем на шаг подключения к серверу
                _connect_timeout.start(75);
            } else {
                setConnectedStep(STEP_NET_CLOSE);
                _connect_timeout.start(25);
            }
        }
        lastReceiveTimeout.start();
    }

    /**
     * Вызывается, если connect_timeout закончился или остановлен.
     * В потомках можно делать дополнительное действия и задавать свои connect_timeout
     */
    virtual void NetStep() {
        switch (connectedStep) {
            case STEP_NET_CLOSE: // отключим сеть
                connectStatus=CS_netOff;
                reconnectServerStep=0;
                if (_connect_timeout.isTimeout()) _connect_timeout.start(2);
                if (IsServerSetup) setConnectedStep(STEP_SERVER_AP);
                else setConnectedStep(STEP_NET_START);
                return;
            case STEP_NET_START: // включим сеть
                if (_connect_timeout.isTimeout()) _connect_timeout.start(2);
                connectStatus=CS_netConnecting;
                setConnectedStep(STEP_NET_CONNECT);
                return;
            case STEP_NET_CONNECT: // подключение и инициализация сети
                if (isConnected(false)) {
                    reconnectServerStep++;
                    if (reconnectServerStep < 7) {
                        connectStatus=CS_netConnected;
                        _connect_timeout.start(1);
                        setConnectedStep(STEP_SERVER_CONNECT);
                    } else
                        onConnectChange(DISCONNECT_CONNECT_SERVER_COUNT);
                } else if (lastReceiveTimeout.isTimeout(10000)) { // максимум ждем 10 сек
#if (LOG_NET>0)
                    MW_LOG_MODULE(this); MW_LOG_LN(F("Error: Net connect timeout!"));
#endif
                    onConnectChange(DISCONNECT_NET_TIMEOUT);
                }
                return;
            case STEP_SERVER_CONNECT: // подключимся к серверу
                _connect_timeout.start(2);
                setConnectedStep(STEP_SERVER_CONNECTING);
                return;
            case STEP_SERVER_CONNECTING: // ждем подключения к серверу
                if (isConnected(true)) {
                    startNewSession();
                    if (frame.SendHandshake()>0) { // отправляем хендшейк в буфер отправки
                        SendAll(); // сразу отправим из буфера отправки
                        _connect_timeout.start(1); // ждем Handshake
                        setConnectedStep(STEP_SERVER_WAIT_ID);
                    }
                }
                if (lastReceiveTimeout.isTimeout(10000)) {
#if (LOG_NET>0)
                    MW_LOG_MODULE(this); MW_LOG_LN(F("Error: Server connect timeout!"));
#endif
                    onConnectChange(DISCONNECT_CONNECT_TIMEOUT);
                }
                return;
            case STEP_SERVER_WAIT_ID: // дождемся кода контроллера в событии EVENT_CHANGE
                if (lastReceiveTimeout.isTimeout(10000)) {
                    onConnectChange(DISCONNECT_CONTROLLER_ID_TIMEOUT);
                }
                return;
            case STEP_SERVER_CONNECTED:
                if (!isConnected(false)) {
                    onConnectChange(DISCONNECT_NET_DONT_CONNECT);
                } else
                if (!isConnected(true)) {
                    onConnectChange(DISCONNECT_SERVER_DONT_CONNECT);
                } else
                if (lastReceiveTimeout.isTimeout(getConnectTimeoutDSec()*100)) { // превышено время от последнего переданного байта
                    onConnectChange(DISCONNECT_RECEIVE_TIMEOUT);
                } else
                if (connectStatus != CS_serverConnecting) { // только подключились
                    connectStatus = CS_serverConnecting;
                    reconnectServerStep=0;
                    onConnectChange(CONNECT_COMMAND);
                }

            default:
                break;
        }

    }

    void setConnectedStep(MWOSNetStep newStep) {
        connectedStep=newStep;
#if (LOG_NET>1)
        MW_LOG_MODULE(this); MW_LOG(F("net step: ")); MW_LOG_LN(connectedStep);
#endif
        lastReceiveTimeout.start();
    }

    /**
     * Обработка событий от сети.
     * @param eventType Тип события
     * @return  Событие обработано успешно.
     */
    virtual bool onNetEvent(NetPacketEvent eventType) {
#if (LOG_RECEIVE>2)
        MW_LOG_TIME(); MW_LOG(F("onNetEvent: ")); MW_LOG(eventType); MW_LOG(' '); receivePacket.printPos();
#endif
        switch (eventType) {
            case NetPacketReceive: {
                frame.serverCommand(); // обработка фрейма
            } break;
            case NetPacketReceiveError: {

            } break;
        }
        if (connectStatus==CS_serverConnecting && frame.startSession) {
            // начнем основное подключение
            connectStatus=CS_serverConnected;
        }
        return true;
    }

    // время пинга [сек/10]
    uint32_t getPingDSec() {
        if (millis()>3600000UL && _pingSecund[1]>_pingSecund[0]) return _pingSecund[1]*10;
        return _pingSecund[0]*10;
    }

    // время таймаута разрыва связи (если долго не было данных)
    uint32_t getConnectTimeoutDSec() {
        return getPingDSec()+100;
    }

    // Модуль занят приемом или передачей
    virtual bool IsBusy() {
        return false; // _timeout.isTimeout();
    }

    /**
     * Этот контроллер в сети этого модуля?
     * @param toCID
     * @return
     */
    bool IsMyCID(uint32_t toCID) {
        for (int i = 0; i < CIDS_LIST_SIZE; ++i)
            if (_listCIDs[i]==toCID) return true;
        return false;
    }

protected:

};
#endif
