#ifndef MWOS3_MWOSNETWIFI_H
#define MWOS3_MWOSNETWIFI_H
/**
 * Модуль связи WiFi для ESP8266 и ESP32
 * Author: vladimir9@bk.ru
 *
 * Неодходимо добавить в platformio.ini
 lib_deps = WiFi
 *
 * Автоматически поддерживает подключение к WiFi
 * ---------------------------------------------
 * 1. если не удалось подключиться к последней сети (или раньше никуда не подключались), пытается подключиться к сети по умолчанию MWOS_TO_WIFI_NAME (если задано)
 * 2. если подключение не удается в течении MW_NoConnectWiFi_Sec [120 сек],
 *   то переводим WiFi в режим точи доступа и запускаем сервер настроек WiFi на MW_ServerWiFi_Minutes [10 минут]
 * 3. если задали новые настройки, то сразу переподключаем WiFi по ним (переходим с этими настройками к п.1)
 * 4. если в течении MW_ServerWiFi_Minutes [10 минут] ничего не задали, то выключем точку доступа и переходим к п.1
 *
 * Таким образом, при невозможности подключения к WiFi через каждые MW_NoConnectWiFi_Sec [120 сек]
 * будет запускаться точка доступа MWOS_WIFI_NAME_ID на MW_ServerWiFi_Minutes [10 минут]
 *
 * Автоматически поддерживает подключение к socket-серверу voscom.ru:8080
 *-----------------------------------------------------------------------
 * автоматически переподключается к серверу при каждом отключении от сервера
 * или если не приходят данные от сервера больше MW_ReciveTimeout_DSec [600 дСек = 60 сек]
 *
 * Во всех режимах переподключения/перенастройки WiFi/сервера, все модули контроллера полноценно функционируют (кроме факта отсутствия связи с сервером)
 */
#include "core/net/MWOSNetDevice.h"
#if defined(ESP8266)
#include "ESP8266WiFi.h"
#endif
#include "MWAsyncClient.h"
#include "MWServerWiFi.h"
#include "core/net/MWOSNetIP.h"
#include <WiFiClientSecure.h>

#ifdef ESP_PLATFORM
// если необходимо использовать SSL-подключение (только для платформы ESP) пока не отлажено!!!
//#define SSL_CERTIFICATE_USE
#endif

#ifndef MWOW_AP_AFTER_NORECONNECTS
#define	MWOW_AP_AFTER_NORECONNECTS 3	// через сколько неудачных подключений к сети WiFi включать режим сервера настроек (если не задан режим MWOS_NO_WIFI_AP_IF_DONT_CONNECT)
#endif

#ifndef MW_ServerWiFi_Minutes
#define	MW_ServerWiFi_Minutes 10	// на сколько минут включется режим сервера настроек
#endif
#ifndef MW_NoConnectWiFi_Sec
#define	MW_NoConnectWiFi_Sec 120
#endif
#ifndef MWOS_TO_WIFI_NAME
#define	MWOS_TO_WIFI_NAME	""
#endif
#ifndef MWOS_TO_WIFI_PASS
#define	MWOS_TO_WIFI_PASS	""
#endif

#ifndef MWOS_WIFI_NAME_ID
#define	MWOS_WIFI_NAME_ID	"voscom"
#endif
#ifndef MWOS_WIFI_PASS
#define	MWOS_WIFI_PASS	NULL
#endif

#ifndef MW_Controller_IP
#define MW_Controller_IP {192,168,0,177}
#endif

#ifndef MWOS_SERVER_HOST
#define MWOS_SERVER_HOST "voscom.online"
#endif

#ifndef MWOS_SERVER_PORT
#define MWOS_SERVER_PORT 8080
#endif

const char * server_ssid = MWOS_WIFI_NAME_ID;
const char * server_password = MWOS_WIFI_PASS;

class MWOSNetWiFi : public MWOSNetIP {
public:

    MWServerWiFi * server=NULL;

    bool ap_mode=false;
#ifdef MWOS_BUTTON_PIN_WIFI_AP
    bool needValueButtonAp=false;
#endif

    uint8_t lastStatusWiFi=0;
    uint8_t lastModeWiFi=0;
    uint8_t countWiFiNoConnect=0;

    String nameWiFi=MWOS_TO_WIFI_NAME;
    String passWiFi=MWOS_TO_WIFI_PASS;


    MWOSNetWiFi() : MWOSNetIP() {
    }

    virtual void onInit() {
        MWOSNetIP::onInit();
#ifdef MWOS_BUTTON_PIN_WIFI_AP
        if (digitalRead(MWOS_BUTTON_PIN_WIFI_AP)==needValueButtonAp) startWiFiAp();
		else
#endif
        stopWiFiAP();
    }

    virtual void onUpdate() {
        uint8_t statusWiFi= WiFi.status();
        uint8_t modeWiFi= WiFi.getMode();
        if (lastStatusWiFi!=statusWiFi) {
            lastStatusWiFi=statusWiFi;
            MW_LOG_MODULE(this); MW_LOG(F("WiFi status: ")); MW_LOG_LN(statusWiFi);
        }
        if (lastModeWiFi!=modeWiFi) {
            lastModeWiFi=modeWiFi;
            MW_LOG_MODULE(this); MW_LOG(F("WiFi mode: ")); MW_LOG_LN(modeWiFi);
        }
        if (statusWiFi != WL_CONNECTED) {
            if (connectedStep>STEP_SERVER_CONNECTING) {
                onDisconnect();
            }
        }
#ifdef MWOS_WIFI_SERIAL_OPTION
        if (MWOS_WIFI_SERIAL_OPTION.available()>6) {
			String optionsFromUSB="";
			while ((MWOS_WIFI_SERIAL_OPTION.available()>0) && (optionsFromUSB.length()<80)) optionsFromUSB+=char(MWOS_WIFI_SERIAL_OPTION.read());
			int n0=optionsFromUSB.indexOf("wifi:");
			if (n0>=0) {
				optionsFromUSB=optionsFromUSB.substring(n0+5);
				int n1=optionsFromUSB.indexOf('#');
				if (n1>0) {
					optionsFromUSB=optionsFromUSB.substring(0,n1);
					int n2=optionsFromUSB.indexOf(';');
					if (n2>0) {
						String optionsFromUSB_param1=optionsFromUSB.substring(0,n2);
						String optionsFromUSB_param2=optionsFromUSB.substring(n2+1);
						if ((optionsFromUSB_param1.length()>0) && (optionsFromUSB_param2.length()>0)) {
							nameWiFi=optionsFromUSB_param1;
							passWiFi=optionsFromUSB_param2;
							onDisconnect();
						}
					}
				}
			}
		}
#endif
        MWOSNetIP::onUpdate();
    }

    virtual void clearConfig() {
        WiFi.begin("0");
        WiFi.disconnect(true); // сбросим настройки WiFi
    }

    virtual void connectToServer() { // начать подключение к серверу
        countWiFiNoConnect=0;
        if (ap_mode) {
            startServer();
            connect_timeout.start(MW_ServerWiFi_Minutes*600);
        } else {
#ifdef SSL_CERTIFICATE_USE
            if (!_stream) {
                connectToServerSSH();
            }
#endif
            if (!_stream) {
                MWAsyncClient * client=new MWAsyncClient((size_t) MWOS_SEND_BUFFER_SIZE);
                _stream=client;
                client->setTimeout((uint32_t) 6000); // таймаут 5 сек, чтобы успеть сбросить вачдог в 8 сек
                MW_LOG_MODULE(this);  MW_LOG(F("MWOSController set stream: ")); MW_LOG_LN((uint16_t) MWOS_SEND_BUFFER_SIZE);
            }
            MWOSNetIP::connectToServer();
        }
    }

#ifdef SSL_CERTIFICATE_USE
    void connectToServerSSH() { // подключиться к серверу с сертификатом

        // root CA can be downloaded in:
        const char* rootCABuff = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIHQjCCBiqgAwIBAgIMS3iMfVp4EAbPgpKIMA0GCSqGSIb3DQEBCwUAMEwxCzAJ\n" \
"aNVAZFD5XHvXlFcKIX+8ag0OYVV/pRmZPvDked7bZfh/CccGqzI=\n" \
"-----END CERTIFICATE-----\n";

        // Fill with your certificate.pem.crt with LINE ENDING
        const char* certificateBuff = \
"-----BEGIN CERTIFICATE-----\n"\
"MIIFFDCCAvwCAQEwDQYJKoZIhvcNAQELBQAwGjEYMBYGA1UEAwwPKi52b3Njb20u\n"\
"MtLisw2vMm0=\n"\
"-----END CERTIFICATE-----\n";

        // Fill with your private.pem.key with LINE ENDING
        const char* privateKeyBuff = \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIJKAIBAAKCAgEAv/1lXMmEweYaEad+Rt5FV6mA+9VPWIFzps/Rzv1aKugoRX6E\n"\
"15v8jUbtrtkPAIvTdNxzP0y0wxXLNLWiw4JeXkC9t7B3/aQss2fYLFK3zrw=\n"\
"-----END RSA PRIVATE KEY-----\n";

        WiFiClientSecure * client=new WiFiClientSecure();
        client->setInsecure(); // без сертификата
        //client->setCACert(rootCABuff); // задать корневой сертификат
        //client->setCertificate(certificateBuff); // for client verification
        //client->setPrivateKey(privateKeyBuff);  // for client verification

        _stream=client;
        client->setTimeout(6000); // таймаут 5 сек, чтобы успеть сбросить вачдог в 8 сек
        MW_LOG_MODULE(this);  MW_LOG(F("WiFiClientSecure set stream: ")); MW_LOG_LN((uint16_t) MWOS_SEND_BUFFER_SIZE);
    }
#endif

    virtual bool isConnectedServer(bool firstTime) { // есть подключение к серверу?
        if (ap_mode) {
            if (server!=NULL) {
                server->handleClient();
                if (server->_ssid!="") {
                    saveWiFi(); // от сервера передали настройки вайфая
                }
                //if (server->_hash!="") {
                //	saveController(); // от сервера передали настройки контроллера
                //}
            }
            return false;
        }
        return MWOSNetIP::isConnectedServer(firstTime);
    }

    virtual void startNet() { // аппаратно включить сеть
        connect_timeout.start(1);
    }

    virtual void connectNet() { // начать подключение к сети
        startWiFi(); // попытаемся подключиться к вайфай
    }

    virtual bool isConnectedNet() { // есть подключение к сети?
        uint8_t statusWiFi= WiFi.status();
        if (ap_mode) {
            return (WiFi.getMode()==3);
        }
        return (statusWiFi==WL_CONNECTED);
    }

    virtual void closeNet() { // закрыть подключение к сети
        MWOSNetIP::onDisconnect();
        stopWiFiAP();
        stopWiFi();
        MWOSNetIP::closeNet();
    }

    virtual void onDisconnect() {
        MWOSNetIP::onDisconnect();
#ifndef	MWOS_NO_WIFI_AP_IF_DONT_CONNECT
        countWiFiNoConnect++;
        MW_LOG_MODULE(this); MW_LOG(F("WiFiNoConnects: ")); MW_LOG_LN(countWiFiNoConnect);
        if (countWiFiNoConnect>MWOW_AP_AFTER_NORECONNECTS) {
            startWiFiAp();
        }
#endif
#ifndef MWOS_WIFI_NOFAST_RECONNECT
        if ((!ap_mode) && (WiFi.status()==WL_CONNECTED)) { // если сейчас не режим сервера и к сети подключились
            MWOSNetIP::onDisconnect();
            setConnectedStep(STEP_NET_CONNECTING); // сразу проверим подключение к сети
            lastReciveTimeout.start(50); // повторим подключение к серверу через 10 сек
            connect_timeout.start(MWOS_NET_CONNECTNET_TIMEOUT);
        }
#endif
    }

    /**
     * Подключиться к запомненной сети
     * Если и запомненной сети нет, то подключается к заданной сети
     * Если и заданной сети нет, то стартует точку доступа
     */
    void startWiFi() {
        if (!connectToSavedWiFi()) {
            if (nameWiFi!="") {
                stopWiFiAP();
                MW_LOG_MODULE(this);  MW_LOG(F("To WiFi connected: ")); MW_LOG_LN(nameWiFi);
                WiFi.begin(nameWiFi.c_str(),passWiFi.c_str());
            } else {
                startWiFiAp();
            }
        }
    }

    /**
     * Подключиться к сохраненной ранее сети
     */
    bool connectToSavedWiFi() {
        if (WiFi.SSID() && (((String) WiFi.SSID()).length()>0)) {
            stopWiFiAP();
            WiFi.begin();
            MW_LOG_MODULE(this); MW_LOG(F("To last WiFi connected: ")); MW_LOG_LN(WiFi.SSID());
            return true;
        }
        return false;
    }

    /**
     * Запустить точку доступа
     */
    void startWiFiAp() {
        if (ap_mode) return;
        ap_mode=true;
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP ( server_ssid, server_password );
        MW_LOG_MODULE(this);  MW_LOG(F("WiFi AP: "));  MW_LOG_LN(server_ssid);
        lastReciveTimeout.start(50);
        connect_timeout.start(200);
        setConnectedStep(STEP_NET_CONNECTING);
        countWiFiNoConnect=0;
    }

    void stopWiFi() {
#if defined(ESP8266)
        //trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        WiFi.disconnect();
        ETS_UART_INTR_ENABLE();
#else
        WiFi.disconnect();
#endif
        WiFi.mode(WIFI_STA);
    }

    void stopWiFiAP() {
        ap_mode=false;
        stopServer();
#if defined(ESP8266)
        //trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
#else
        esp_wifi_disconnect();
#endif
        MW_LOG_MODULE(this);  MW_LOG_LN(F("WiFi AP stop!"));
        // WiFi.disconnect();
        WiFi.mode(WIFI_STA);
    }

    void saveWiFi() {
        MW_LOG_MODULE(this); MW_LOG_LN(F("WiFi AP saveWiFi!"));
        if (!server) return;
        //if ((server->_ip!="") && (server->_gw!="") && (server->_sn!="")) {

        //}
        if (server->_ssid) {
            nameWiFi=server->_ssid;
            passWiFi=server->_pass;
            onDisconnect();
        }
    }

    void startServer() {
        server=new MWServerWiFi();
        server->startServer();
        MW_LOG_MODULE(this); MW_LOG_LN(F("WiFi AP Server started!"));
    }

    void stopServer() {
        MW_LOG_MODULE(this); MW_LOG_LN(F("WiFi AP Server stoped!"));
        if (server==NULL) return;
        delete server;
        server=NULL;
    }

};


#endif //MWOS3_MWOSNETWIFI_H
