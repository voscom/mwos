/*
 * MWServerWiFi.h
 *
 *  Created on: 19 сент. 2018 г.
 *      Author: mw
 */

#ifndef MWLIB_MWSERVERWIFI_H_
#define MWLIB_MWSERVERWIFI_H_
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

#if defined(ESP8266)
extern "C" {
  #include "user_interface.h"
}
#define ESP_getChipId()   (ESP.getChipId())
#else
#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#endif

const char MW_HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>WiFi settings</title>";
const char MW_HTTP_STYLE[] PROGMEM           = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
const char MW_HTTP_SCRIPT[] PROGMEM          = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";

const char MW_HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
const char MW_HTTP_PORTAL_OPTIONS[] PROGMEM  = "<form action=\"/cnt\" method=\"get\"><button>Configure controller</button></form><br/><form action=\"/wifi\" method=\"get\"><button>Configure WiFi</button></form><br/><form action=\"/0wifi\" method=\"get\"><button>Configure WiFi (No Scan)</button></form><br/>";
const char MW_HTTP_ITEM[] PROGMEM            = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char MW_HTTP_FORM_START[] PROGMEM      = "<form method='get' action='wifisave'><input id='s' name='s' length=32 placeholder='SSID'><br/>";
const char MW_HTTP_FORM_PASSW[] PROGMEM      = "<br/><input id='s' name='s' length=32 placeholder='SSID'><br/><input id='p' name='p' length=64 type='password' placeholder='password'>";
const char MW_HTTP_FORM_ABOUT[] PROGMEM      = "<br/><input id='ab' name='ab' length=50 placeholder='About controller'>";
const char MW_HTTP_FORM_HASH[] PROGMEM       = "<br/><input id='ha' name='ha' length=16 placeholder='Hash (16-hex)'>";
const char MW_HTTP_FORM_END[] PROGMEM        = "<br/><button type='submit'>save</button></form>";
const char MW_HTTP_SCAN_LINK[] PROGMEM       = "<br/><div class=\"c\"><a href=\"/wifi\">Scan</a></div>";
const char MW_HTTP_SAVED[] PROGMEM           = "<div>Credentials Saved<br />Trying to connect Weread to network.<br />If it fails reconnect to AP to try again</div>";
const char MW_HTTP_END[] PROGMEM             = "</div></body></html>";

class MWServerWiFi : public WebServer {
public:

	String _ssid = ""; // код wifi-сети для автоматического подключения
	String _pass = ""; // пароль wifi-сети для автоматического подключения

	// Данные контроллера для static ip
	String _ip = ""; // static ip
	String _gw = ""; // static gateway
	String _sn = ""; // static netmask

	// настройки контроллера
	String _hash = "";	// хеш контроллера
	String _about = "";	// место нахождения (краткое описание) контроллера

	MWServerWiFi();
	void startServer();

private:
    void          handleRoot();
    void          handleWifi(boolean scan);
    void          handleWifiSave();
    void          handleController();


};

#endif
#endif /* MWLIB_MWSERVERWIFI_H_ */
