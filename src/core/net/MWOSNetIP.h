#ifndef MWOS3_MWOSNETIP_H
#define MWOS3_MWOSNETIP_H

#ifndef MWOS_SEND_BUFFER_SIZE
#define MWOS_SEND_BUFFER_SIZE 2048 // размер буффера отправки для сетевого устройства
#endif

const char server_url1[] PROGMEM = "voscom.online";
const char server_url2[] PROGMEM = "voscom.ru";
const char server_url3[] PROGMEM = "voscom.ddns.net";

#ifndef MWOS_SERVER_PORT
#define MWOS_SERVER_PORT 8081
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
    }

    virtual void onInit() {
        MWOSNetDevice::onInit();
        reconnectCount=loadValue(2,&p_reconnectCount,0);
        if (reconnectCount==0 || reconnectCount>200) reconnectCount=2;
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
        if (!_stream) {
            MWAsyncClient * client=new MWAsyncClient(MWOS_SEND_BUFFER_SIZE);
            _stream=client;
            client->setTimeout(6000); // таймаут 5 сек, чтобы успеть сбросить вачдог в 8 сек
            MW_LOG_MODULE(this);  MW_LOG(F("MWOSController set stream: ")); MW_LOG_LN((uint16_t) MWOS_SEND_BUFFER_SIZE);
        }
        reconnectStep++;
        if (reconnectStep>reconnectCount+2) reconnectStep=0;
        if (reconnectStep<reconnectCount) {
            if (ipPort==0 || ipPort==0xffffffff) {
                connectAsync(IPAddress(ip4)); // подключимся к сокет-серверу из параметра
            } else {
                connectAsync(MWOS_SERVER_HOST); // подключимся к сокет-серверу по умолчанию
            }
        } else {
            if (reconnectStep==reconnectCount) {
                char server_url[strlen_P(server_url1) + 1];
                strcpy_P(server_url, server_url1);
                connectAsync(server_url); // подключимся к сокет-серверу voscom.online
            }
            if (reconnectStep==reconnectCount+1) {
                char server_url[strlen_P(server_url2) + 1];
                strcpy_P(server_url, server_url2);
                connectAsync(server_url); // подключимся к сокет-серверу
            }
            if (reconnectStep==reconnectCount+2) {
                char server_url[strlen_P(server_url3) + 1];
                strcpy_P(server_url, server_url3);
                connectAsync(server_url); // подключимся к сокет-серверу
            }
        }
        connect_timeout.start(50); // следующая попытка только через 5 сек
        MWOSNetDevice::connectToServer();
    }

    void connectAsync(const char *server_url){
        ((MWAsyncClient *) _stream)->connect(server_url,ipPort); // подключимся к сокет-серверу
        MW_LOG_MODULE(this); MW_LOG(F("Connect to server url ")); MW_LOG(reconnectStep); MW_LOG('>'); MW_LOG(server_url); MW_LOG(':'); MW_LOG_LN(ipPort);
    }

    void connectAsync(IPAddress ip){
        ((MWAsyncClient *) _stream)->connect(ip,ipPort); // подключимся к сокет-серверу
        MW_LOG_MODULE(this); MW_LOG(F("Connect to server ip ")); MW_LOG(reconnectStep); MW_LOG('>'); MW_LOG(ip); MW_LOG(':'); MW_LOG_LN(ipPort);
    }

    virtual bool isConnectedServer() { // есть подключение к серверу?
        return ((MWAsyncClient *) _stream)->connected();
    }

    void disconnect() {
        if (_stream!=NULL) {
            if (((MWAsyncClient *) _stream)->connected()) ((MWAsyncClient *) _stream)->stop(); // разорвем соединение, чтобы начать его заново
            //delete _stream;
            //_stream=NULL;
            MW_LOG_MODULE(this); MW_LOG(F("disconnect step:"));  MW_LOG_LN(reconnectStep);
        }
    }

};


#endif //MWOS3_MWOSNETIP_H
