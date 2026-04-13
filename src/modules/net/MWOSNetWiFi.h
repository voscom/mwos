#ifndef MWOS3_MWOSNETWIFI_H
#define MWOS3_MWOSNETWIFI_H
/**
 * Модуль связи WiFi для ESP8266 и ESP32
 * Author: vladimir9@bk.ru
 *
 * Необходимо добавить в platformio.ini
 lib_deps = WiFi
 *
 * Автоматически поддерживает подключение к WiFi
 * Во всех режимах переподключения/перенастройки WiFi/сервера, все модули контроллера полноценно функционируют (кроме факта отсутствия связи с сервером)
 */

#include "core/net/esp32/EspAsyncWiFiClient.h"
#include "core/net/MWOSNetIP.h"
#if defined(ESP8266)
#include "ESP8266WiFi.h"
#endif
#include "modules/net/core/MWAsyncClient.h"
#include "modules/net/core/MWSetupWiFi.h"
#include "core/net/MWOSNetIP.h"
#include <WiFiClientSecure.h>

#ifndef MWOS_WIFI_BUFFER_SIZE
// буфер пакетов для связи по wifi (поделен поровну, между приемом и передачей)
#define	MWOS_WIFI_BUFFER_SIZE 1024*16
#endif

#ifndef MWOS_CLIENT_SEND_BUFFER_SIZE
// буфер отправки клиента WiFI
#define	MWOS_CLIENT_SEND_BUFFER_SIZE 1024*4
#endif

#ifndef MWOS_CLIENT_RCV_BUFFER_SIZE
// буфер приема клиента WiFI
#define	MWOS_CLIENT_RCV_BUFFER_SIZE 1024*4
#endif

#ifndef MWOS_AP_COUNT
// через сколько неудачных подключений к сети WiFi включать режим точки доступа с сервером настроек (0 - никогда)
#define	MWOS_AP_COUNT 0
#endif

#ifndef MWOS_AP_MINUTES
// на сколько минут включается режим сервера настроек
#define	MWOS_AP_MINUTES 20
#endif

#ifndef WL1
// подключаться к этой сети WiFi
#define	WL1	"voscom"
#endif
#ifndef WP1
// подключаться с этим паролем WiFi
#define	WP1	"12345678"
#endif

#ifndef WL2
// имя точки доступа для подключения к серверу настроек
#define	WL2	"voscom"
#endif
#ifndef WP2
// пароль точки доступа для подключения к серверу настроек
#define	WP2	"12345678"
#endif

class MWOSNetWiFi : public MWOSNetIP {
public:

    // ssid (имя сети) WiFi
    MWOS_PARAM(20, ssid, PARAM_STRING, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NVS, 50);
    // пароль WiFi
    MWOS_PARAM(21, pass, PARAM_STRING, PARAM_TYPE_OPTIONS+PARAM_TYPE_SECRET, MWOS_STORAGE_NVS, 16);

    MWSetupWiFi * server=nullptr;
    uint8_t lastStatusWiFi=0;
    uint8_t lastModeWiFi=0;
    uint8_t countWiFiNoConnect=0;

    MWOSNetWiFi(uint16_t sizeBuffer=MWOS_WIFI_BUFFER_SIZE, uint8_t * buffer=nullptr) : MWOSNetIP(nullptr,sizeBuffer,buffer) {
        net_module_type=Net_WiFi_ESP;
        maxPacketSize=min(min(MWOS_CLIENT_SEND_BUFFER_SIZE/2,MWOS_WIFI_BUFFER_SIZE/4),8100); // максимальный пакет - вполовину буфера отправки
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
        if (modeEvent==EVENT_UPDATE && connectedStep!=STEP_NET_OFF) { // Вызывается каждый тик операционной системы
            uint8_t statusWiFi= WiFi.status();
            uint8_t modeWiFi= WiFi.getMode();
            if (lastStatusWiFi!=statusWiFi) {
                lastStatusWiFi=statusWiFi;
#if (LOG_NET>0)
                MW_LOG_MODULE(this); MW_LOG(F("WiFi status: ")); MW_LOG_LN(statusWiFi);
#endif
            }
            if (lastModeWiFi!=modeWiFi) {
                lastModeWiFi=modeWiFi;
#if (LOG_NET>0)
                MW_LOG_MODULE(this); MW_LOG(F("WiFi mode: ")); MW_LOG_LN(modeWiFi);
#endif
            }
            if (statusWiFi != WL_CONNECTED) {
                if (connectedStep>STEP_SERVER_CONNECTING) {
                    onConnectChange(DISCONNECT_WIFI_NO_CONNECT);
                }
            }
            if (server) server->update();
        } else
        if (modeEvent==EVENT_INIT) { // инициализация
            MWOSNetIP::onEvent(modeEvent,data);
            loadValueString(WL1, p_ssid);
            loadValueString(WP1, p_pass);
            if (IsStaticIP()) {
                WiFi.config(getStaticIP(), getStaticGateway(), getStaticSubnet(), getStaticDNS());
            }
            return;
        }
        MWOSNetIP::onEvent(modeEvent,data);
    }

    /**
     * Вызывается, если connect_timeout закончился или остановлен.
     * В потомках можно делать дополнительное действия и задавать свои connect_timeout
     */
    virtual void NetStep() {
        switch (connectedStep) {
            case STEP_NET_CLOSE: closeWiFi(); break;
            case STEP_NET_START: startWiFi(); break;
            case STEP_SERVER_CONNECT: connectToServer(); break;
        }
        MWOSNetIP::NetStep();
    }

    void clearConfig() {
        WiFi.begin("0");
        WiFi.disconnect(true); // сбросим настройки WiFi
    }

    void connectToServer() { // начать подключение к серверу
        countWiFiNoConnect=0;
        if (IsServerSetup) {
            setConnectedStep(STEP_SERVER_AP);
            startServer();
            _connect_timeout.start(MWOS_AP_MINUTES*470); // в минуте 60/0.128 = 470 тиков
        } else {
            if (!_stream) {
                //EspAsyncWiFiClient * client=new EspAsyncWiFiClient(MWOS_CLIENT_SEND_BUFFER_SIZE, MWOS_CLIENT_RCV_BUFFER_SIZE);
                MWAsyncClient * client=new MWAsyncClient(MWOS_CLIENT_SEND_BUFFER_SIZE);
                _stream=client;
                client->setTimeout((uint32_t) 6000); // таймаут, чтобы успеть сбросить вачдог в 8 сек
#if (LOG_NET>1)
                MW_LOG_MODULE(this);  MW_LOG(F("MWOSController set stream: ")); MW_LOG_LN((uint16_t) MWOS_CLIENT_SEND_BUFFER_SIZE);
#endif

            }
        }
    }

    virtual bool isConnected(bool toServer) { // есть подключение к серверу?
        if (!toServer) { // статус подключения к сети
            uint8_t statusWiFi= WiFi.status();
            if (IsServerSetup) {
                uint8_t wfmode=(uint8_t) WiFi.getMode();
#if (LOG_NET>3)
                MW_LOG_MODULE(this); MW_LOG(F("modeWiFi: ")); MW_LOG(wfmode); MW_LOG(F(", status: ")); MW_LOG_LN((uint8_t) statusWiFi);
#endif
#ifndef WIFI_EVENT_APSTA
#define WIFI_EVENT_APSTA WIFI_STA
#endif
#ifndef WIFI_EVENT_AP
#define WIFI_EVENT_AP WIFI_AP
#endif
                return (wfmode==WIFI_EVENT_APSTA || wfmode==WIFI_EVENT_AP);
            }
            return (statusWiFi==WL_CONNECTED);
        }
        if (IsServerSetup) {
            if (server) {
                server->handleClient();
                if (server->_ssid!="") {
                    saveWiFi(); // от сервера передали настройки вайфая
                }
            }
            return false;
        }
        return MWOSNetIP::isConnected(toServer);
    }

    void closeWiFi() { // закрыть подключение к сети
        if (IsNetOff) { // это полная деактивация сети
            onConnectChange(DISCONNECT_WIFI_NET_CLOSE);
            stopWiFiAP();
            stopWiFi();
            WiFi.disconnect(true);
#if (LOG_NET>0)
            MW_LOG_MODULE(this); MW_LOG_LN(F("IsNetOff!"));
#endif
            setConnectedStep(STEP_NET_OFF);
        } else {
            onConnectChange(DISCONNECT_WIFI_NET_CLOSE);
            stopWiFiAP();
            stopWiFi();
        }
    }

    virtual void onConnectChange(MWOSConnectCode connectCode) {
        if (connectCode < CONNECT_COMMAND) {
#if	MWOS_AP_COUNT>0
            countWiFiNoConnect++;
#if (LOG_NET>0)
            MW_LOG_MODULE(this); MW_LOG(F("WiFiNoConnects: ")); MW_LOG_LN(countWiFiNoConnect);
#endif
            if (countWiFiNoConnect>MWOS_AP_COUNT) {
                startWiFiAp();
                return;
            }
#endif
        }
        MWOSNetIP::onConnectChange(connectCode);
    }

    /**
     * Подключиться к запомненной сети
     * Если и запомненной сети нет, то подключается к заданной сети
     * Если и заданной сети нет, то стартует точку доступа
     */
    void startWiFi() {
        if (IsServerSetup) {
            startWiFiAp();
            return;
        }
        if (server) stopServer();
        if (WiFi.begin(loadValueString(WL1, p_ssid),loadValueString(WP1, p_pass))!=WL_CONNECTED) {
            //startWiFiAp();
        }
    }

    /**
     * Запустить точку доступа
     */
    void startWiFiAp() {
        IsServerSetup=true;
        connectStatus=CS_netAPConnecting;
        //WiFi.mode(WIFI_AP_STA);
        WiFi.softAP ( WP2, WL2 );
// IP-адрес точки доступа
        IPAddress local_ip(192, 168, 4, 1); // IP ESP32
        IPAddress gateway(192, 168, 4, 1); // Шлюз
        IPAddress subnet(255, 255, 255, 0); // Маска подсети
        WiFi.softAPConfig(local_ip, gateway, subnet);
        MW_LOG_MODULE(this);  MW_LOG(F("WiFi AP: "));  MW_LOG_LN(WP2);
        setConnectedStep(STEP_NET_CONNECT);
        lastReceiveTimeout.start();
        _connect_timeout.start(300);
        countWiFiNoConnect=0;
    }

    void stopWiFi() {
#if defined(ESP8266)
        //trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        WiFi.disconnect();
        ETS_UART_INTR_ENABLE();
#else
        if (WiFi.isConnected()) WiFi.disconnect();
#endif
        MW_LOG_MODULE(this);  MW_LOG_LN(F("stopWiFi"));
    }

    void stopWiFiAP() {
        IsServerSetup=false;
        stopServer();
#if defined(ESP8266)
        //trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
#else
        WiFi.softAPdisconnect();
#endif

        MW_LOG_MODULE(this);  MW_LOG_LN(F("WiFi AP stop!"));
    }

    void saveWiFi() {
        MW_LOG_MODULE(this); MW_LOG_LN(F("WiFi AP saveWiFi!"));
        if (!server) return;
        if (server->_ssid) {
            saveValueString(server->_ssid,p_ssid,-1);
            saveValueString(server->_pass,p_pass,-1);
            stopWiFiAP();
            onConnectChange(DISCONNECT_WIFI_AP_SAVE);
            WiFi.begin((server->_ssid,server->_pass)); // сохраним новые значения
            setConnectedStep(STEP_NET_CLOSE);
        }
    }

    void startServer() {
        WiFi.softAP ( WL2, WP2 ); // еще раз объявим точку доступа
// IP-адрес точки доступа
        IPAddress local_ip(192, 168, 4, 1); // IP ESP32
        IPAddress gateway(192, 168, 4, 1); // Шлюз
        IPAddress subnet(255, 255, 255, 0); // Маска подсети
        WiFi.softAPConfig(local_ip, gateway, subnet);

        server=new MWSetupWiFi();
        server->start();
        MW_LOG_MODULE(this); MW_LOG_LN(F("WiFi AP Server started!"));
        connectStatus=CS_netAPConnected;
    }

    void stopServer() {
        MW_LOG_MODULE(this); MW_LOG_LN(F("WiFi AP Server stoped!"));
        if (!server) return;
        server->stop();
        delete server;
        server=nullptr;
    }

};


#endif //MWOS3_MWOSNETWIFI_H
