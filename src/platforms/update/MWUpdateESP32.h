#ifndef MWUPDATEESP_H
#define MWUPDATEESP_H
#ifdef ESP32

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include "core/MWOSModule.h"
#include "core/adlib/firmware.h"

#ifndef ASYNC_UPDATE
#ifdef ARDUINO_ESP32S3_DEV
// включить асинхронное обновление
#define ASYNC_UPDATE 1
#else
// отключить асинхронное обновление
#define ASYNC_UPDATE 0
#endif
#endif
// показывать лог обновления прошивки (0-4)
//#define LOG_UPDATE 4

/**
 * Обновление прошивки для ESP32
 * Параметр firware - дата прошивки в linuxtime.
 * Параметр autoUpdate [1] - включено автообновление раз в сутки (через каждые 23-25 часов)
 *
 * Команды для немедленной проверки прошивки и автоматической перепрошивки, если есть обновления.
 * firmware:firmware = 1 - обновить все *.cfg (и если не отключен autoUpdate то через 5-10 сек само обновит прошивку)
 * firmware:firmware = 2 - обновить прошивку без *.cfg
 *
 * Обычный порядок обновления прошивки:
 * 1. Запрашивает whatsnews.txt
 * 2. Передает и запрашивает по очереди файлы:
    defaults.cfg
    t{серия}.cfg
    {cid контроллера}.cfg
    u{cid контроллера}.cfg
 * 3. Вызывает обновление прошивки.
 */

class MWUpdateESP32 {
public:
    // сейчас идет ожидание ответа на запрос обновления
    uint8_t waitUpdate:1;
    // используется https (иначе - http)
    uint8_t _https:1;
    // последняя операция закончена с ошибкой
    uint8_t _lastError:1;
    uint8_t :5;

    MWOSUpdateESP32() {
        _https=true;
    }

   /**
     * Асинхронно запустить апдейт прошивки
     */
    void StartUpdate() {
#if (LOG_UPDATE>2)
        MW_LOG_MODULE(this); MW_LOG_LN(F("StartUpdate"));
#endif
        waitUpdate=true;
#if (ASYNC_UPDATE>0)
        xTaskCreate(
            asyncStartUpdate,   // Функция задачи
            "HTTPS_update",  // Имя задачи
            8192,        // Размер стека
            this,        // Параметры
            1,           // Приоритет
            NULL    //&taskHandle     // Сохраняем хэндл задачи
        );
#else
        AsyncStartUpdate();
#endif
    }

private:
    // defaults.cfg, t{controllerType}.cfg, {cid}.cfg, u{cid}.cfg, whatsnew.txt
    //TaskHandle_t taskHandle = NULL;

#if (ASYNC_UPDATE>0)
    static void asyncStartRequestFile(void* pvParameters) {
        MWOSUpdateESP32* instance = static_cast<MWOSUpdateESP32*>(pvParameters);
        instance->AsyncStartRequestFile();
        vTaskDelete(NULL);
    }

    static void asyncStartUpdate(void* pvParameters) {
        MWOSUpdateESP32* instance = static_cast<MWOSUpdateESP32*>(pvParameters);
        instance->AsyncStartUpdate();
        vTaskDelete(NULL);
    }
#endif

    void AsyncStartUpdate() {
#if (LOG_UPDATE>1)
        MW_LOG_MODULE(this); MW_LOG_LN(F("AsyncStartUpdate"));
#endif
        String urlStr=getUrl(0);
#if (LOG_UPDATE>1)
        MW_LOG_MODULE(this); MW_LOG(F("FIRMWARE: ")); MW_LOG_LN(urlStr);
#endif
        HTTPClient http;
        if (_https) {
            WiFiClientSecure client;
            setHTTPS(&client);
            http.begin(client, urlStr); // Передаем клиент с SSL
        } else {
            http.begin( urlStr); // Передаем клиент без SSL
        }
        t_httpUpdate_return ret = httpUpdate.update(http);
        http.end();
#if (LOG_UPDATE>0)
        switch (ret) {
            case HTTP_UPDATE_FAILED: MW_LOG_TIME(); MW_LOG(F("HTTP_UPDATE_FAILED Error: ")); MW_LOG_LN(httpUpdate.getLastErrorString()); break;
            case HTTP_UPDATE_NO_UPDATES: MW_LOG_TIME(); MW_LOG_LN(F("HTTP_UPDATE_NO_UPDATES")); break;
            case HTTP_UPDATE_OK: MW_LOG_TIME(); MW_LOG_LN(F("HTTP_UPDATE_OK")); break;
        }
#endif
        if (ret==HTTP_UPDATE_OK) {
            nvs_flash_erase(); // почистим настройки
            ESP.restart(); // при успешном обновлении - перезагрузим
        } else {
            _lastError=true;
            waitUpdate=false;
        }
    }

    /**
     * Задать подключение HTTPS для клиента.
     * Если в первой файловой системе есть файл сертификата /voscom.cer, то задает сертификат из него.
     * Иначе - HTTPS без сертификата.
     * @param client    Клиент
     */
    void setHTTPS(WiFiClientSecure * client) {
         client->setInsecure(); // без сертификата
    }

    String getSite(IPAddress ip) {
        _https=false;
        return String(F("http://"))+ip.toString()+"/mwos3";
    }

    String getSite(String site) {
        _https=true;
        return String(F("https://"))+site;
    }

    String getUrl(int stepA) {
        if (stepA<0 || stepA>5) return "";
        uint32_t cid=CID;
        String res=getSite(MWOS_SERVER_HOST)+String(F("/firmware.php?v="))+getVersionBuild()+ // String(F("&XDEBUG_SESSION_START=PHPSTORM"))+
            "&t="+MWOS_CONTROLLER_TYPE+"&p="+MWOS_PROJECT+ // "&l="+String(MWOSNetModule::getNetModule()->getLID())+
            "&b="+MWOS_BOARD_FULL+"&c="+String(MWOS_BUILD_TIME)+"&s="+String(MWOS_USER_HASH,HEX)+"&id="+String(cid);
        if (stepA>0) res+="&a="+stepA;
        return res;
    }


};



#endif
#endif
