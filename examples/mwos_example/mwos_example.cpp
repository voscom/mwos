#include "MWOSConfig.h"
#include <MWOS.h>
#include <MWOSTime.h>
#include <MWOSKey.h>
#include <MWOSKeyExt.h>
#include "MWOSSensorBin.h"
#include "MWOSSensorADC.h"
#include "MWOSSensorDS.h"
//#include "MWOSEncoder.h"
#include "MWOSPWM.h"


// можно задать различные справочные данные контроллера в формате JSON
const char controllerInfo[] PROGMEM = {
        "{'name':'Теплица','obj':'2','about':'Управление теплицей'}"
};

// добавляем модули
// обязательно надо добавить любой модуль связи! Все остальные модули - не обязательно.
#if  defined(ESP32) || defined(ESP8266)
#include "MWOSNetWiFi.h"
MWOSNetWiFi net; // Модуль связи по WiFi
#else
#include "MWOSNetW5500.h"
MWOSNetW5500 net;  // Модуль связи по W5500
#endif
MWOSTime time0; // часы
MWOSKey<3> key; // 4 ключа с обычной функциональностью
MWOSKeyExt<1> key1; // 1 ключ с расширенной функциональностью
MWOSSensorBin<4> sensor0; // 4 датчика бинарных
MWOSSensorADC<4> adc0; // 4 датчика аналоговых
MWOSSensorDS<4> ds; // датчики DS (до 4 шт) на линии dds_pin
//MWOSPWM hz0; // генератор частоты
//MWOSEncoder encoder0(); // энкодер

void setup() {

    Serial.begin(115200);
    delay(5000);
#ifdef MWOS_DEBUG
    //delay(5000); // подождем, чтобы можно было успеть открыть монитор для отладки
    //MWEEPROM.clear();
#endif

#ifdef ESP32
    Serial.print("Total heap: ");
    Serial.println(ESP.getHeapSize());
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Total PSRAM: ");
    Serial.println(ESP.getPsramSize());
    Serial.print("Free PSRAM: ");
    Serial.println(ESP.getFreePsram());
#endif

    mwos.name=(char *) &controllerInfo; // дабавим информацию о контроллере (это может быть просто название контроллера или json с значениями полей для таблицы БД)
    mwos.start(); // запустить mwos

    key._invert=true; // по умолчанию (если EEPROM стерт) - инвертируем значения ключей
    key1._invert=true; // по умолчанию (если EEPROM стерт) - инвертируем значения ключей

    //attachInterrupt()
}

void loop() {
    mwos.update(); // передаем управление mwos
}
