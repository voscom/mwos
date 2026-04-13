#ifndef MWOS3_MWOSNETIP_H
#define MWOS3_MWOSNETIP_H

#include "Client.h"
#include "core/net/MWOSNet.h"

// можно прописать до 3-х альтернативных адресов сервера
#ifdef MWOS_SERVER_HOST1
const char server_url1[] PROGMEM = MWOS_SERVER_HOST1;
#endif
#ifdef MWOS_SERVER_HOST2
const char server_url2[] PROGMEM = MWOS_SERVER_HOST2;
#endif
#ifdef MWOS_SERVER_HOST3
const char server_url3[] PROGMEM = MWOS_SERVER_HOST3;
#endif

//#define STATIC_IP {192,168,0,54}
//#define STATIC_GATEWAY {192,168,0,1}
//#define STATIC_SUBNET {255,255,255,0}
//#define STATIC_DNS {192,168,0,1}

#ifndef MWOS_SERVER_PORT
#define MWOS_SERVER_PORT 8081
#endif

#ifndef MWOS_SERVER_HOST
#define MWOS_SERVER_HOST "mwos.voscom.online"
#endif

#ifndef CNM
#define CNM "Net"
#endif

/**
 * Модуль связи IP
 * Author: vladimir9@bk.ru
 *
 * Подключение к IP-сокет-серверу
 * Если параметр ipPort==0 или ipPort=0xffff (порт сброшен или не задан) то задает ipPort=MWOS_SERVER_PORT (порт по умолчанию)
 * 1. Если задан параметр ip4 пытается подключиться к серверу ip4:ipPort
 * Если не ip4 задан, то вместо ip4 использует MWOS_SERVER_HOST:ipPort
 * Если не удалось
 * 2. Повторяет подключение к ip4:ipPort (или MWOS_SERVER_HOST:ipPort) reconnectCount раз
 * 3. Пытается подключиться к voscom.online:ipPort
 * 4. Пытается подключиться к voscom.ru:ipPort
 * 5. Пытается подключиться к voscom.ddns.net:ipPort
 *
 * MWOS_SERVER_HOST и MWOS_SERVER_PORT могут задаваться в MWOSConfig.h
 * По умолчанию - voscom.online:88
 *
 */
class MWOSNetIP : public MWOSNet {
public:

    // тип подключения
    MWOS_PARAM_FF(11, dhcp, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1, "DHCP;STATIC IP4");
    // static IP
    MWOS_PARAM(12, staticIP, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 4);
    // static gateway
    MWOS_PARAM(13, gateway, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 4);
    // static subnet
    MWOS_PARAM(14, subnet, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 4);
    // static dns
    MWOS_PARAM(15, dns, PARAM_UINT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 4);
    // сетевое имя контроллера (справочная информация)
    MWOS_PARAM(16, name, PARAM_STRING, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NVS, 50);

    /**
     * Инициализация сетевого модуля. Можно задать сразу или позже _stream и sendPacket.init(). Если задать буфер (или только размер), то он будет пополам поделен на прием и передачу.
     * @param stream        Ввод/вывод
     * @param sizeBuffer    Суммарный размер буфера приема и передачи. Обязательно - четный, что-бы ровно поделить пополам.
     * @param buffer        Внешний буфер приема и передачи, если не задан - будет создан в куче
     */
    MWOSNetIP(void * stream=nullptr, uint16_t sizeBuffer=0, uint8_t * buffer=nullptr) : MWOSNet(stream,sizeBuffer,buffer) {
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
        if (modeEvent==EVENT_INIT) { // инициализация
            loadValueString(CNM, p_name,-1); // зададим имя сети по умолчанию
        }
        MWOSNet::onEvent(modeEvent,data);
    }

    virtual void onConnectChange(MWOSConnectCode connectCode) {
        if (connectCode>=CONNECT_COMMAND) {

        } else {
            if (_stream) { // разорвем соединение, чтобы начать его заново
                ((Client *)_stream)->stop();
            }
        }
        MWOSNet::onConnectChange(connectCode);
    }

    /**
     * Вызывается, если connect_timeout закончился или остановлен.
     * В потомках можно делать дополнительное действия и задавать свои connect_timeout
     */
    virtual void NetStep() {
        switch (connectedStep) {
            case STEP_SERVER_CONNECT: connectToServerClient(); break;
        }
        MWOSNet::NetStep();
    }

    void connectToServerClient() {
        // начать подключение к серверу
#ifdef MWOS_SERVER_HOST1
        if (reconnectServerStep==4) {
            char server_url[strlen_P(server_url1) + 1];
            strcpy_P(server_url, server_url1);
            connectAsync(server_url); // подключимся к сокет-серверу voscom.online
        } else
#endif
#ifdef MWOS_SERVER_HOST2
        if (reconnectServerStep==5) {
            char server_url[strlen_P(server_url2) + 1];
            strcpy_P(server_url, server_url2);
            connectAsync(server_url); // подключимся к сокет-серверу voscom.online
        } else
#endif
#ifdef MWOS_SERVER_HOST3
        if (reconnectServerStep==6) {
            char server_url[strlen_P(server_url3) + 1];
            strcpy_P(server_url, server_url3);
            connectAsync(server_url); // подключимся к сокет-серверу voscom.online
        } else
#endif
        {
            connectAsync(MWOS_SERVER_HOST); // подключимся к сокет-серверу по умолчанию
        }
    }

    bool IsStaticIP() {
#ifdef STATIC_IP
        uint8_t dhcp = loadValueInt(1, &p_dhcp, 0);
#else
        uint8_t dhcp = loadValueInt(0, p_dhcp, 0);
#endif
        return dhcp>0;
    }

    IPAddress getStaticAddr(MWOSParam * param, IPAddress defIP) {
        IPAddress staticIP(loadValueInt(defIP[0], *param, 0), loadValueInt(defIP[1], *param, 1), loadValueInt(defIP[2], *param, 2), loadValueInt(defIP[3], *param, 3));
        return staticIP;
    }

    IPAddress getStaticIP() {
#ifdef STATIC_IP
        IPAddress defStaticIP(STATIC_IP);
#else
        IPAddress defStaticIP({0,0,0,0});
#endif
        return getStaticAddr(&p_staticIP,defStaticIP);
    }

    IPAddress getStaticGateway() {
#ifdef STATIC_GATEWAY
        IPAddress defStaticIP(STATIC_GATEWAY);
#else
#ifdef STATIC_IP
        IPAddress defStaticIP(STATIC_IP);
        defStaticIP[3]=1;
#else
        IPAddress defStaticIP({0,0,0,0});
#endif
#endif
        return getStaticAddr(&p_gateway,defStaticIP);
    }

    IPAddress getStaticDNS() {
#ifdef STATIC_DNS
        IPAddress defStaticIP(STATIC_DNS);
#else
#ifdef STATIC_IP
        IPAddress defStaticIP(STATIC_IP);
        defStaticIP[3]=1;
#else
        IPAddress defStaticIP({0,0,0,0});
#endif
#endif
        return getStaticAddr(&p_dns,defStaticIP);
    }

    IPAddress getStaticSubnet() {
#ifdef STATIC_SUBNET
        IPAddress defStaticIP(STATIC_SUBNET);
#else
        IPAddress defStaticIP({255,255,255,0});
#endif
        return getStaticAddr(&p_subnet,defStaticIP);
    }

    void connectAsync(const char *server_url){
#if (LOG_NET>1)
        MW_LOG_MODULE(this); MW_LOG(F("Connect to server url step: "));
#endif
        ((Client *) _stream)->connect(server_url,MWOS_SERVER_PORT); // подключимся к сокет-серверу
#if (LOG_NET>1)
        MW_LOG(reconnectServerStep); MW_LOG('>'); MW_LOG(server_url); MW_LOG(':'); MW_LOG_LN(MWOS_SERVER_PORT);
#endif
    }

    void connectAsync(IPAddress ip){
#if (LOG_NET>1)
        MW_LOG_MODULE(this); MW_LOG(F("Connect to server ip step: "));
#endif
        ((Client *) _stream)->connect(ip,MWOS_SERVER_PORT); // подключимся к сокет-серверу
#if (LOG_NET>1)
        MW_LOG(reconnectServerStep); MW_LOG('>'); MW_LOG(ip); MW_LOG(':'); MW_LOG_LN(MWOS_SERVER_PORT);
#endif
    }

    virtual bool isConnected(bool toServer) { // есть подключение к серверу?
        if (toServer) return ((Client *) _stream)->connected();
        return MWOSNet::isConnected(toServer);
    }

};


#endif //MWOS3_MWOSNETIP_H
