#ifndef MWOS3_MWOSNETCONTROLLER_H
#define MWOS3_MWOSNETCONTROLLER_H

#include "MWOSNetModule.h"
/***
 * Модуль работы с сетью через устройство, требующего подключения к сети
 * 1. настраивает устройство (и пересбрасывает, если нужно)
 * 2. подключается к сети
 * 3. подключается к серверу
 *
 */

#ifndef MWOS_User_Hash
#define MWOS_User_Hash 0
#endif

#ifndef MWOS_NET_RESTART_NORMAL_SIGNAL
#define MWOS_NET_RESTART_NORMAL_SIGNAL 1	// уровень сигнала RESET для нормального функционирования сети
#endif

#ifndef MWOS_NET_RESTART_TIMEOUT
#define MWOS_NET_RESTART_TIMEOUT 10 // [сек/10] время аппаратного сброса сети (шаг 0)
#endif

#ifndef MWOS_NET_STARTNET_TIMEOUT
#define MWOS_NET_STARTNET_TIMEOUT 50 // [сек/10] таймаут ожидания для включения сети (шаг 2)
#endif

#ifndef MWOS_NET_BEFORE_CONNECT_TIMEOUT
#define MWOS_NET_BEFORE_CONNECT_TIMEOUT 50 // [сек/10] время ожидания после включения сети перед проверкой подключения к сети (шаг 4)
#endif

#ifndef MWOS_NET_CONNECTNET_TIMEOUT
#define MWOS_NET_CONNECTNET_TIMEOUT 200 // [сек/10] таймаут ожидания для подключения к сети (шаг 4)
#endif

#ifndef MWOS_NET_CONNECT_SERVER_TIMEOUT
#define MWOS_NET_CONNECT_SERVER_TIMEOUT 200 // [сек/10] таймаут ожидания подключения к серверу (шаг 6)
#endif

#ifndef MWOS_NET_HANDSHAKE_TIMEOUT
#define MWOS_NET_HANDSHAKE_TIMEOUT 100 // [сек/10] таймаут ожидания хендшейка после подключения к серверу (шаг 7)
#endif

#ifndef MWOS_NET_CONTROLLER_ID_TIMEOUT
#define MWOS_NET_CONTROLLER_ID_TIMEOUT 100 // [сек/10] таймаут ожидания распределения кода контроллера после хендшейка (шаг 9)
#endif

#ifndef MWOS_NET_DISCONNECT_TIMEOUT
#define MWOS_NET_DISCONNECT_TIMEOUT 150 // [сек/10] таймаут ожидания перед повторым переподключением после разрыва связи
#endif

const char MWOS_NET_MODULE_NAME[] PROGMEM = { "net" };

class MWOSNetDevice : public MWOSNetModule {
public:
#pragma pack(push,1)
    MWTimeout connect_timeout;
#ifdef MWOS_NET_RESTART_ERRORS
    uint8_t errorConnectCount=0;
#endif
#pragma pack(pop)

    MWOSNetDevice(char * unit_name=(char *) &MWOS_NET_MODULE_NAME) : MWOSNetModule(unit_name) {
    }

    virtual void connectedStepRun() {
        if (connectedStep!=15) {
            if (IsConnected)  {
                onDisconnect();
            }
        }
        switch (connectedStep) {
            case 0: // отключим подключение к сети
                IsConnected= false;
                connect_timeout.start(MWOS_NET_RESTART_TIMEOUT);
                closeNet();
                setConnectedStep(1);
                return;
            case 1: // ждем отключения сети
                if (!connect_timeout.isTimeout()) return;
                setConnectedStep(2);
            case 2: // аппаратно включим сеть
                connect_timeout.start(MWOS_NET_STARTNET_TIMEOUT);
                setConnectedStep(3);
                startNet();
                return;
            case 3: // ждем аппаратного включения сети
                if (!connect_timeout.isTimeout()) return;
                setConnectedStep(4);
            case 4: // подключение и инициализация сети
                lastReciveTimeout.start(MWOS_NET_BEFORE_CONNECT_TIMEOUT);
                connect_timeout.start(MWOS_NET_CONNECTNET_TIMEOUT);
                setConnectedStep(5);
                connectNet();
                return;
            case 5: // ждем подключения сети
                if (!lastReciveTimeout.isTimeout()) return; // ждем таймаута по любому
                if (isConnectedNet()) {
                    setConnectedStep(6);
                } else if (connect_timeout.isTimeout()) {
                    onDisconnect(); // отвалиться через 20 сек, если не подключились
                }
                return;
            case 6: // подключимся к серверу
                connect_timeout.start(MWOS_NET_CONNECT_SERVER_TIMEOUT); // переподключать только через 20 сек
                setConnectedStep(7);
                connectToServer();
                return;
            case 7: // ждем подключения к серверу
                if (isConnectedServer()) {
                    setConnectedStep(8);
                    connect_timeout.start(MWOS_NET_HANDSHAKE_TIMEOUT); // ждем Handshake 10сек
                }
                else if (connect_timeout.isTimeout()) onDisconnect(); // отвалиться через 20 сек, если не подключились
                return;
            case 8: // отправляем хендшейк
                if (writeHandshake(cid)>0) {
                    setConnectedStep(9);
                    connect_timeout.start(MWOS_NET_CONTROLLER_ID_TIMEOUT); // отвалиться через несколько сек, если не пришел код контроллера
                    lastReciveTimeout.stop(); // остановим таймаут приемки данных (его стартует при получении пакета от сервера)
                }
                else if (connect_timeout.isTimeout()) onDisconnect();// отвалиться через 10 сек, если не прошел Handshake
                return;
            case 9: // дождемся кода контроллера
                if ((_stream!=NULL) && (cid>0) && lastReciveTimeout.isStarted()) {
                    setConnectedStep(15);
                }
                else if (connect_timeout.isTimeout()) onDisconnect();// отвалиться через 20 сек, если не пришел код контроллера
                return;
            case 15:
                if (!IsConnected) {
                    onConnect();
                } else {
                    if (lastReciveTimeout.isTimeout()) { // провышено время от последнего переданного байта
                        onReciveTimeout();
                    }
                    if (!isConnectedServer()) {
                        MW_LOG_MODULE(this);  MW_LOG_LN(F("Server dont connect!"));
                        onDisconnect();
                        return;
                    }
                }
        }
    }

    virtual void connectToServer() { // начать подключение к серверу
#ifdef MWOS_SERVER_HOST
        MW_LOG_MODULE(this); MW_LOG(F("connect to server: ")); MW_LOG_ADDR(MWOS_SERVER_HOST,MWOS_SERVER_PORT);
#endif
    }

    virtual bool isConnectedServer() { // есть подключение к серверу?
        return true;
    }

    virtual void connectNet() { // начать подключение к сети
    }

    virtual bool isConnectedNet() { // есть подключение к сети?
        return true;
    }

    virtual void startNet() { // аппаратно включить сеть
#ifdef MWOS_NET_RESTART_PIN
        digitalWrite(MWOS_NET_RESTART_PIN,MWOS_NET_RESTART_NORMAL_SIGNAL); // закончим аппаратный сброс
        MW_LOG_MODULE(this); MW_LOG_LN(F("net restart stop"));
#endif
    }

    virtual void closeNet() { // аппаратно выключить сеть
#ifdef MWOS_NET_RESTART_PIN
        bool resetSignal=!MWOS_NET_RESTART_NORMAL_SIGNAL;
		digitalWrite(MWOS_NET_RESTART_PIN,resetSignal); // начнем аппаратный сброс
		MW_LOG_MODULE(this); MW_LOG(F("net restart start: ")); MW_LOG_LN(resetSignal);
#endif
    }

    virtual void onConnect() {
        MWOSNetModule::onConnect();
#ifdef MWOS_NET_RESTART_ERRORS
        errorConnectCount=0;
#endif
    }

    virtual void onDisconnect() {
        MWOSNetModule::onDisconnect();
#ifdef MWOS_NET_RESTART_ERRORS
        errorConnectCount+=1;
		if (errorConnectCount>MWOS_NET_RESTART_ERRORS) {
            MW_LOG_MODULE(this); MW_LOG(F("restart after: ")); MW_LOG_LN(MWOS_NET_RESTART_ERRORS-errorConnectCount);
		    resetController();
		}
#endif
        connect_timeout.start(MWOS_NET_DISCONNECT_TIMEOUT); // подключать не сразу после разрыва
    }



};


#endif //MWOS3_STM32F103_MWOSNETCONTROLLER_H
