#ifndef MWOS3_MWOSNETIP_H
#define MWOS3_MWOSNETIP_H

#include "Client.h"

const char server_url1[] PROGMEM = "voscom.online";
const char server_url2[] PROGMEM = "voscom.ru";
const char server_url3[] PROGMEM = "voscom.ddns.net";

#ifndef MWOS_SERVER_PORT
#define MWOS_SERVER_PORT 8082
#endif

#ifndef MWOS_SERVER_HOST
#define MWOS_SERVER_HOST "voscom.online"
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
class MWOSNetIP : public MWOSNetDevice {
public:

#pragma pack(push,1)
    uint8_t reconnectStep=0;
    uint8_t reconnectCount=0;
    uint16_t ip4=0;
    uint32_t ipPort=0;
#pragma pack(pop)

    // количество неудачных подключений, до попытки получения нового адреса
    MWOS_PARAM(1, reconnectCount, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, 1);
    MWOS_PARAM(2, ip4, mwos_param_uint32, mwos_param_option, mwos_param_storage_eeprom, 1);
    MWOS_PARAM(3, ipPort, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);

    MWOSNetIP() : MWOSNetDevice() {
        AddParam(&p_reconnectCount);
        AddParam(&p_ip4);
        AddParam(&p_ipPort);
    }

    virtual void onInit() {
        MWOSNetDevice::onInit();
        reconnectCount=loadValue(2,&p_reconnectCount,0);
        if (reconnectCount==0 || reconnectCount>200) reconnectCount=3;
        ip4=loadValue(0xffffffff,&p_ip4,0);
        ipPort=loadValue(MWOS_SERVER_PORT,&p_ipPort,0);
        if (ipPort<80 || ipPort==0xffff) ipPort=MWOS_SERVER_PORT;
        reconnectStep=0xff; // начать подключение с начала списка
    }

    virtual void onConnect() {
        MWOSNetDevice::onConnect();
        reconnectStep=0xff; // начать подключение с начала списка
    }

    virtual void connectToServer() { // начать подключение к серверу
        reconnectStep++;
#ifndef MWOS_NOT_RECONNECT_URLS
        if (reconnectStep>reconnectCount+3) reconnectStep=0;
#else
        if (reconnectStep>reconnectCount) reconnectStep=0;
#endif
        if (reconnectStep<reconnectCount) {
            if (ipPort==0 || ipPort==0xffffffff) {
                connectAsync(IPAddress(ip4)); // подключимся к сокет-серверу из параметра
            } else {
                connectAsync(MWOS_SERVER_HOST); // подключимся к сокет-серверу по умолчанию
            }
        } else {
#ifndef MWOS_NOT_RECONNECT_URLS
            if (reconnectStep==reconnectCount+1) {
                char server_url[strlen_P(server_url1) + 1];
                strcpy_P(server_url, server_url1);
                connectAsync(server_url); // подключимся к сокет-серверу voscom.online
            } else
            if (reconnectStep==reconnectCount+2) {
                char server_url[strlen_P(server_url2) + 1];
                strcpy_P(server_url, server_url2);
                connectAsync(server_url); // подключимся к сокет-серверу
            } else
            if (reconnectStep==reconnectCount+3) {
                char server_url[strlen_P(server_url3) + 1];
                strcpy_P(server_url, server_url3);
                connectAsync(server_url); // подключимся к сокет-серверу
            } else
#endif
            {
                char server_url[strlen_P(server_url1) + 1];
                strcpy_P(server_url, server_url1);
                connectAsync(MWOS_SERVER_HOST); // подключимся к сокет-серверу по умолчанию
            }
        }

        connect_timeout.start(50); // следующая попытка только через 5 сек
        MWOSNetDevice::connectToServer();
    }

    void connectAsync(const char *server_url){
        MW_LOG_MODULE(this); MW_LOG(F("Connect to server url step: "));
        ((Client *) _stream)->connect(server_url,ipPort); // подключимся к сокет-серверу
        MW_LOG(reconnectStep); MW_LOG('>'); MW_LOG(server_url); MW_LOG(':'); MW_LOG_LN(ipPort);
    }

    void connectAsync(IPAddress ip){
        MW_LOG_MODULE(this); MW_LOG(F("Connect to server ip step: "));
        ((Client *) _stream)->connect(ip,ipPort); // подключимся к сокет-серверу
        MW_LOG(reconnectStep); MW_LOG('>'); MW_LOG(ip); MW_LOG(':'); MW_LOG_LN(ipPort);
    }

    virtual bool isConnectedServer(bool firstTime) { // есть подключение к серверу?
        return ((Client *) _stream)->connected();
    }

    virtual void onDisconnect() {
        if (_stream!=NULL) { // разорвем соединение, чтобы начать его заново
            ((Client *)_stream)->stop();
            //delete _stream;
            //_stream=NULL;
            MW_LOG_MODULE(this); MW_LOG(F("disconnect step:"));  MW_LOG(reconnectStep); MW_DEBUG_LOG_MEM(false);
        }
        MWOSNetDevice::onDisconnect();
    }

};


#endif //MWOS3_MWOSNETIP_H
