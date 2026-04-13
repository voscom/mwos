#ifndef MWOW_MWSETUPWIFI_H_
#define MWOW_MWSETUPWIFI_H_
#if defined(ESP8266) || defined(ESP32)

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#define  WebServer ESP8266WebServer
#else
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <memory>
#include <DNSServer.h>

const char MW_HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>WiFi settings</title>";
const char MW_HTTP_STYLE[] PROGMEM           = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
const char MW_HTTP_SCRIPT[] PROGMEM          = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char MW_HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-blockRX;min-width:260px;'>";
const char MW_HTTP_ITEM[] PROGMEM            = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char MW_HTTP_FORM_START[] PROGMEM      = "<form method='get' action='wifisave'><br/>";
const char MW_HTTP_FORM_PASSW[] PROGMEM      = "<br/><input id='s' name='s' length=32 placeholder='SSID'><br/><input id='p' name='p' length=64 type='password' placeholder='password'>";
const char MW_HTTP_FORM_END[] PROGMEM        = "<br/><button type='submit'>save</button></form>";
const char MW_HTTP_SCAN_LINK[] PROGMEM       = "<br/><div class=\"c\"><a href=\"/wifi\">Scan</a></div>";
const char MW_HTTP_END[] PROGMEM             = "</div></body></html>";
const char MW_HTTP_SAVED[] PROGMEM           = "<div>Credentials Saved<br />Reconnecting to saved WiFi...<br />If it fails reconnect to AP to try again</div>";

/**
 * Android автоматически проверяет наличие Captive Portal при подключении к Wi-Fi. Для этого он отправляет запрос на http://connectivitycheck.android.com/generate_204.
Если ваш роутер перехватывает этот запрос и возвращает HTTP-код 302 (редирект) на ваш сайт, Android откроет браузер с вашим сайтом.
 */


/**
 * HTTP-Сервер для настройки WiFi
 * SSID и пароль
 * Заданные настройки пользователем передаются в поля _ssid и _pass
 */
class MWSetupWiFi : public WebServer {
public:

	String _ssid = ""; // код wifi-сети для автоматического подключения
	String _pass = ""; // пароль wifi-сети для автоматического подключения

    const byte DNS_PORT = 53; // Порт DNS-сервера
    DNSServer * dnsServer;

    MWSetupWiFi() : WebServer(80) {

    }

    void start() {
        if (dnsServer==NULL) dnsServer=new DNSServer();
        IPAddress local_ip(192, 168, 4, 1); // IP ESP32
        dnsServer->start(DNS_PORT, "*", local_ip);
        _ssid="";
        _pass="";
        on("/", std::bind(&MWSetupWiFi::handleRoot, this));
        on("/wifi", std::bind(&MWSetupWiFi::handleWiFi, this));
        on("/wifisave", std::bind(&MWSetupWiFi::handleWifiSave, this));
        // http://connectivitycheck.android.com/generate_204
        on("/generate_204", std::bind(&MWSetupWiFi::handleRedirect, this));
        onNotFound(std::bind(&MWSetupWiFi::handleRoot, this));
        begin();
    }

    void stop() {
        dnsServer->stop();
        delete dnsServer;
        dnsServer=NULL;
        close();
    }

    void update() {
        if (dnsServer!=NULL) dnsServer->processNextRequest(); // Обработка DNS-запросов
    }

protected:

    void handleRedirect() {
        Serial.println("\n\rhandleRedirect!");
        // Возвращаем HTTP-код 302 (редирект) на указанный IP
        sendHeader("Location", "http://192.168.4.1/", true);
        send(302, "text/plain", "");
    }

    void handleRoot() {
        Serial.println("\n\rhandleRoot!");
        httpRoot(false);
    }

    void handleWiFi() {
        httpRoot(true);
    }

    void handleWifiSave() {
        _ssid = arg("s");
        _pass = arg("p");
        MW_LOG_TIME(); MW_LOG(F("WifiSave: ")); MW_LOG(_ssid); MW_LOG(';'); MW_LOG_LN(_pass);

        String page = FPSTR(MW_HTTP_HEAD);
        page.replace("{v}", "Credentials Saved");
        page += FPSTR(MW_HTTP_SCRIPT);
        page += FPSTR(MW_HTTP_STYLE);
        //page += _customHeadElement;
        page += FPSTR(MW_HTTP_HEAD_END);
        page += FPSTR(MW_HTTP_SAVED);
        page += FPSTR(MW_HTTP_END);

        sendHeader("Content-Length", String(page.length()));
        send(200, "text/html", page);

    }

    void httpRoot(bool scan) {
        String page = FPSTR(MW_HTTP_HEAD);
        page.replace("{v}", "Setup WiFi");
        page += FPSTR(MW_HTTP_SCRIPT);
        page += FPSTR(MW_HTTP_STYLE);
        //page += _customHeadElement;
        page += FPSTR(MW_HTTP_HEAD_END);

        if (scan) {
            int n = WiFi.scanNetworks();
            //DEBUG_WM(F("Scan done"));
            if (n == 0) {
                //DEBUG_WM(F("No networks found"));
                page += F("No networks found. Refresh to scan again.");
            } else {

                //sort networks
                int indices[n];
                for (int i = 0; i < n; i++) {
                    indices[i] = i;
                }

                // RSSI SORT

                // old sort
                for (int i = 0; i < n; i++) {
                    for (int j = i + 1; j < n; j++) {
                        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                            std::swap(indices[i], indices[j]);
                        }
                    }
                }

                /*std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
                  {
                  return WiFi.RSSI(a) > WiFi.RSSI(b);
                  });*/

                // remove duplicates ( must be RSSI sorted )
                String cssid;
                for (int i = 0; i < n; i++) {
                    if (indices[i] == -1) continue;
                    cssid = WiFi.SSID(indices[i]);
                    for (int j = i + 1; j < n; j++) {
                        if (cssid == WiFi.SSID(indices[j])) {
                            //DEBUG_WM("DUP AP: " + WiFi.SSID(indices[j]));
                            indices[j] = -1; // set dup aps to index -1
                        }
                    }
                }

                //display networks in page
                for (int i = 0; i < n; i++) {
                    if (indices[i] == -1) continue; // skip dups
                    //DEBUG_WM(WiFi.SSID(indices[i]));
                    //DEBUG_WM(WiFi.RSSI(indices[i]));
                    int RSSI=WiFi.RSSI(indices[i]);
                    int quality = 0;
                    if (RSSI <= -100) {
                        quality = 0;
                    } else if (RSSI >= -50) {
                        quality = 100;
                    } else {
                        quality = 2 * (RSSI + 100);
                    }
                    String item = FPSTR(MW_HTTP_ITEM);
                    String rssiQ;
                    rssiQ += quality;
                    item.replace("{v}", WiFi.SSID(indices[i]));
                    item.replace("{r}", rssiQ);
#if defined(ESP8266)
                    if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
#else
                    if (WiFi.encryptionType(indices[i]) != WIFI_AUTH_OPEN) {
#endif
                        item.replace("{i}", "l");
                        //DEBUG_WM(item);
                        page += item;
                        delay(0);
                    } else {
                        //DEBUG_WM(F("Skipping due to quality"));
                    }

                }
                page += "<br/>";
            }
        }

        page += FPSTR(MW_HTTP_FORM_START);
        page += FPSTR(MW_HTTP_FORM_PASSW);
        page += FPSTR(MW_HTTP_FORM_END);
        page += FPSTR(MW_HTTP_SCAN_LINK);

        page += FPSTR(MW_HTTP_END);

        sendHeader("Content-Length", String(page.length()));
        send(200, "text/html", page);
        //DEBUG_WM(F("Sent config page"));
    }


};

#endif
#endif /* MWOW_MWSETUPWIFI_H_ */
