/*
 * MWServerWiFi.cpp
 *
 *  Created on: 19 сент. 2018 г.
 *      Author: mw
 */

#if defined(ESP8266) || defined(ESP32)
#include "MWServerWiFi.h"

MWServerWiFi::MWServerWiFi() : WebServer(80) {


}

void MWServerWiFi::startServer() {
	Serial.println(F("MWServerWiFi startServer..."));
	on("/", std::bind(&MWServerWiFi::handleRoot, this));
	on("/cnt", std::bind(&MWServerWiFi::handleController, this));
	on("/wifi", std::bind(&MWServerWiFi::handleWifi, this, true));
	on("/0wifi", std::bind(&MWServerWiFi::handleWifi, this, false));
	on("/wifisave", std::bind(&MWServerWiFi::handleWifiSave, this));
	begin();
	Serial.println(F("MWServerWiFi startServer!"));
}

void MWServerWiFi::handleRoot() {
  //if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
  //  return;
  //}
	Serial.println(F("AP user: /"));

  String page = FPSTR(MW_HTTP_HEAD);
  page.replace("{v}", "Options");
  page += FPSTR(MW_HTTP_SCRIPT);
  page += FPSTR(MW_HTTP_STYLE);
  //page += _customHeadElement;
  page += FPSTR(MW_HTTP_HEAD_END);
  page += F("<h1>MWOS</h1>");
  page += F("<h3>WiFi</h3>");
  page += FPSTR(MW_HTTP_PORTAL_OPTIONS);
  page += FPSTR(MW_HTTP_END);

  sendHeader("Content-Length", String(page.length()));
  send(200, "text/html", page);

}

void MWServerWiFi::handleController() {
	  String page = FPSTR(MW_HTTP_HEAD);
	  page.replace("{v}", "Config controller");
	  page += FPSTR(MW_HTTP_SCRIPT);
	  page += FPSTR(MW_HTTP_STYLE);
	  page += FPSTR(MW_HTTP_HEAD_END);
	  page += F("<h1>MWOS</h1>");
	  page += F("<h3>Controller</h3>");

	  page += FPSTR(MW_HTTP_FORM_START);
	  page += FPSTR(MW_HTTP_FORM_END);

	  page += FPSTR(MW_HTTP_FORM_HASH);
	  page += FPSTR(MW_HTTP_FORM_ABOUT);

	  page += FPSTR(MW_HTTP_END);
	  sendHeader("Content-Length", String(page.length()));
	  send(200, "text/html", page);
}

void MWServerWiFi::handleWifi(boolean scan) {
	Serial.println(F("AP user: /wifi"));
	  String page = FPSTR(MW_HTTP_HEAD);
	  page.replace("{v}", "Config ESP");
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

void MWServerWiFi::handleWifiSave() {
	Serial.println(F("AP user: /wifisave"));
	  //DEBUG_WM(F("WiFi save"));
	_ssid = arg("s");
	_pass = arg("p");
	_about = arg("ab");
	_hash = arg("ha");
	//MW_LOG(F("Server save: ")); MW_LOG(_ssid); MW_LOG(','); MW_LOG_LN(_pass);

	if (arg("ip") != "") _ip = arg("ip"); // static ip
	if (arg("gw") != "") _gw = arg("gw"); // static gateway
	if (arg("sn") != "") _sn = arg("sn"); // static netmask

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

	  //DEBUG_WM(F("Sent wifi save page"));

	  //connect = true; //signal ready to connect/reset
}
#endif

