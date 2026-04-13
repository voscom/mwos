#include "version.h" // обязательно подключаем версию MWOS !!! Она автоматически генерируется при компиляции.
#define DEBUG 1 // отладочная информация через USB
#define PAUSE 4000 // пауза перед запуском mwos (удобно, если необходимо подключать терминал до начала вывода)

#define MWOS_PROJECT "mwos3.example" // проект.подпроект (проект - обязательно из списка созданных на сервере, подпроект - любой)
#define MWOS_CONTROLLER_ID 0 // id контроллера по умолчанию (если не выдано сервером MWOS). Если 0, то автоматически получает ID
#define MWOS_USER_HASH 0x68E409F2E7500E51	// MWOS-хеш пользователя на центральном сервере
#define WL1 "mwos"	        // название сети WiFi к которой пытаться присоединиться по умолчанию (до того, как задали в настройках)
#define WP1 "mwos1234"	    // пароль сети WiFi (минимум 8 символов) к которой пытаться присоединиться по умолчанию (до того, как задали в настройках)

#include <MWOS.h>

#include "modules/net/MWOSNetWiFi.h"// Подключение модуля WiFi
#include"modules/sensor/core/MWOSSensorFilter.h"// Подключение модуля фильтрации данных датчиков
MWOSNetWiFi netWiFi; // Модуль связи по WiFi
#include "modules/time/MWOSTime.h"// Подключение модуля времени
MWOSTime time0; // часы

#include "fs/MWLittleFS.h"// Подключение файловой системы LittleFS
MWLittleFS littleFS;
#include "modules/fs/MWOSFS.h"// Подключение модуля файловой системы
MWOSFS fs0(&littleFS);

#include "modules/fs/MWOSFile.h"// Подключение модуля работы с файлами для littleFS
MWOSFile fileFS(&fs0);

#include "modules/fs/MWOSUpdateESP32.h"// Подключение модуля обновления прошивки для ESP32
MWOSUpdateESP32 updateESP32;

#include "modules/sensor/MWOSSensorBin.h"// Подключение модуля бинарного датчика
MWOSSensorBin<5> sensorBin;
#include "modules/sensor/MWOSSensorADC.h"// Подключение модуля ADC датчика
MWOSSensorADC<5> sensor4;

#include "modules/key/MWOSKey.h"// Подключение модуля бинарного выхода
const uint8_t keyPorts[] PROGMEM = {GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4}; // порты бинарных выходов
MWOSKey<3> key((uint8_t *) keyPorts, MW_PIN_OUTPUT); //  модуль 3-х однотипных бинарных выходов


#include "modules/key/MWOSKeyAnalog.h"// Подключение модуля аналогового выхода
const uint8_t keyAnalogPorts[] PROGMEM = {GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4}; // порты аналоговых выходов
MWOSKeyAnalog<3> keyAnalog((uint8_t *) keyAnalogPorts, MW_PIN_OUTPUT_PWM); //  модуль аналоговых выходов ШИМ
#include"modules/key/MWOSKeySchedule.h"// Подключение модуля расписания кнопок
MWOSKeySchedule<1,MWOSKeyAnalog<1>> keySchedule(GPIO_NUM_5, MW_PIN_OUTPUT_PWM); //  модуль аналоговых выходов ШИМ
#include"modules/key/MWOSKeySwitch.h"// Подключение модуля переключателя состояния выходов, в зависимости от состояния датчика
// 5 аналоговых переключателей с расписанием (порты можно задать позже - удаленно)
MWOSKeySwitch<5,MWOSKeySchedule<5,MWOSKeyAnalog<5>>> keySwitch;

#include "modules/key/MWOSKeyFeedback.h"// Подключение модуля регулятора с обратной связи
#include"modules/key/MWOSHzESP32.h"// Подключение модуля генератора частоты ESP32
// 5 регуляторов с обратной связью и генератором частоты
MWOSKeyFeedback<5,MWOSHzESP32<5>> keyFeedback;

#include "modules/fs/MWOSCmd.h"// Подключение модуля команд
MWOSCmd cmdOS; // командный модуль для обработки текстовых команд из терминала

void setup() {
    Serial.begin(115200);// Инициализация последовательного порта
    netWiFi.start();// Запуск модуля WiFi
    mwos.start();// Запуск ядра MWOS
}

void loop() {
    mwos.update();// Обновление ядра MWOS
}
