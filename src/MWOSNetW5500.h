#ifndef MWOS3_MWOSNETW5500_H
#define MWOS3_MWOSNETW5500_H
/**
 * Модуль связи через LAN W5500
 * Author: vladimir9@bk.ru
 *
 * Неодходимо добавить в platformio.ini
 lib_deps =
        https://gitlab.com/free_mwos/ethernet
 *
 * Автоматически поддерживает подключение к серверу
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
#include "core/net/MWOSNetIP.h"
#include "core/net/w5500/AsyncEthernetClientW5500.h"
//#define W5500_ETHERNET_SHIELD
#define MWOS_SEND_BUFFER_USE 1 // использовать кольцевой буффер отправки
#include <Ethernet2.h>

#ifndef MWOS_NET_SS
#define MWOS_NET_SS SS
#endif

class MWOSNetW5500 : public MWOSNetIP {
public:

#pragma pack(push,1)
    uint8_t mac[6]= {0xF0,0x01,'m','w',0,0}; // MAC (последние 2 байта изменяются автоматически, чтобы избежать совпадений внутри сети)
#pragma pack(pop)

    MWOS_PARAM(10, pinsW5500, MWOS_PIN_INT_PTYPE, mwos_param_pin+mwos_param_readonly, mwos_param_storage_no, 4);

    MWOSNetW5500() : MWOSNetIP() {
        AddParam(&p_pinsW5500); // добавим пины W5500, что-бы было видно, что они заняты
    }

    virtual void onInit() {
        MWOSNetIP::onInit();
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param==&p_pinsW5500) {
            if (arrayIndex==0) return MOSI;
            if (arrayIndex==1) return MISO;
            if (arrayIndex==2) return SCK;
            if (arrayIndex==3) return MWOS_NET_SS;
        }
        return MWOSNetIP::getValue(param,arrayIndex);
    }

    virtual void connectToServer() { // начать подключение к серверу
        if (!_stream) {
            AsyncEthernetClientW5500 * client=new AsyncEthernetClientW5500(1);
            _stream=(Stream *) client;
            //client->setTimeout((uint32_t) 6000); // таймаут 5 сек, чтобы успеть сбросить вачдог в 8 сек
            MW_LOG_MODULE(this);  MW_LOG(F("MWOSController set stream: ")); MW_LOG((uint16_t) MWOS_SEND_BUFFER_SIZE);  MW_DEBUG_LOG_MEM(false);
        }
        MWOSNetIP::connectToServer();
    }


    virtual void connectNet() {// подключение и инициализация сети
        uint8_t * id_cn=(uint8_t *) &cid;
        mac[4] = id_cn[0];
        mac[5] = id_cn[1];
        MW_LOG_MODULE(this); MW_LOG(F("MAC")); MW_LOG_BYTES((unsigned char *) &mac,6);
        MW_LOG(F(", MOSI:")); MW_LOG(MOSI); MW_LOG(F(", MISO:")); MW_LOG(MISO);
        MW_LOG(F(", SCK:")); MW_LOG(SCK); MW_LOG(F(", SS:")); MW_LOG_LN(MWOS_NET_SS);
        Ethernet.init(MWOS_NET_SS);
#ifndef MWOS_NET_STATIC_IP
        Ethernet.begin(mac);
#else
        Ethernet.begin(mac,MWOS_NET_STATIC_IP);
#endif
        MW_LOG_MODULE(this); MW_LOG(F("Ethernet:")); MW_LOG(Ethernet.localIP()); MW_LOG(';'); MW_LOG(Ethernet.subnetMask()); MW_LOG(';');
            MW_LOG(Ethernet.gatewayIP()); MW_LOG(';'); MW_LOG_LN(Ethernet.dnsServerIP());
        MWOSNetIP::connectNet();
    }

    virtual bool isConnectedNet() { // есть подключение к сети?
        Ethernet.maintain();
        return true;
    }

    virtual void closeNet() { // аппаратно выключить сеть
        MWOSNetIP::closeNet();
    }

    virtual bool isConnectedServer(bool firstTime) { // есть подключение к серверу?
        if (firstTime) return (( AsyncEthernetClientW5500 *) _stream)->status() == SnSR::ESTABLISHED;
        return MWOSNetIP::isConnectedServer(firstTime);
    }


};


#endif //MWOS3_MWOSNETW5500_H
