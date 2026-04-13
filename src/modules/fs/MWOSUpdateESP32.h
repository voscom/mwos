#ifndef MWOS35_MWOSUPDATEESP32_H
#define MWOS35_MWOSUPDATEESP32_H

#ifdef ESP32
#define NO_GLOBAL_HTTPUPDATE
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "core/MWOSModule.h"
#include "core/adlib/firmware.h"
#include "mbedtls/aes.h"

#ifndef ASYNC_UPDATE
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3)
// включить асинхронное обновление
#define ASYNC_UPDATE 1
#else
// отключить асинхронное обновление
#define ASYNC_UPDATE 0
#endif
#endif

// показывать лог обновления прошивки (0-4)
//#define LOG_UPDATE 4

const uint8_t AES_KEY[] = MWOS_UPDATE_CRYPT_KEY;
const uint8_t AES_NONCE[] = MWOS_UPDATE_CRYPT_NONCE;

using MWOSUpdateProgressCB = std::function<void(int, int)>;

/**
* Обновление прошивки для ESP32 (Версия MWOS3.5)
*
* Параметр firmware - дата прошивки в linuxtime.
* Параметр autoUpdate [1] - включено автообновление раз в сутки (через каждые 23-25 часов)
*
* Команды для немедленной проверки прошивки и автоматической перепрошивки, если есть обновления.
* firmware:firmware = 1 - обновить все *.cfg (и если не отключен autoUpdate то через 5-10 сек само обновит прошивку)
* firmware:firmware = 2 - обновить прошивку без *.cfg
*
* Обычный порядок обновления прошивки:
* 1. Запрашивает whatsnews.txt
* 2. Передает и запрашивает по очереди файлы:
*    defaults.cfg
*    t{серия}.cfg
*    {cid контроллера}.cfg
*    u{cid контроллера}.cfg
* 3. Вызывает обновление прошивки.
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - инициализация модуля.
*   EVENT_UPDATE - тиковая обработка (проверка обновлений).
*   EVENT_SET_VALUE - обработка команд установки значений.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSUpdateESP32 : public MWOSModule {
public:
    // --- Локальные переменные ---

    // включено автообновление раз в сутки (через каждые 23-25 часов)
    uint8_t _autoUpdate : 1;
    // сейчас идет ожидание ответа на запрос обновления
    uint8_t waitUpdate : 1;
    // используется https (иначе - http)
    uint8_t _https : 1;
    // последняя операция закончена с ошибкой
    uint8_t _lastError : 1;
    // текущий шаг обновления
    uint8_t stepUpdate : 4;

    MWOSUpdateProgressCB onStart = NULL;
    MWOSUpdateProgressCB onProgress = NULL;
    MWOSUpdateProgressCB onEnd = NULL;
    MWTimeout<uint32_t,15,true> _timeoutUpdate;

    // --- Объявление параметров (автоматическая регистрация) ---

    // статус модуля (дата прошивки)
    MWOS_PARAM(0, firmware, PARAM_INT64, PARAM_TYPE_CONTROL, MWOS_STORAGE_NO, 1);
    // включено автообновление раз в сутки (через каждые 23-25 часов)
    MWOS_PARAM(1, autoUpdate, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    MWOSUpdateESP32() : MWOSModule((char *) F("firmware")) {
        moduleType = MODULE_UNDEFINED;
        waitUpdate = false;
        _autoUpdate = true;
        _https = true;
        _lastError = false;
        stepUpdate = 0;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация
            case EVENT_INIT: {
                _timeoutUpdate.start(100); // от 10 до 15 сек
                _autoUpdate = MWOSModule::loadValueInt(_autoUpdate, p_autoUpdate, 0);
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                _autoUpdate = MWOSModule::loadValueInt(_autoUpdate, p_autoUpdate, 0);
            } break;

            // 3. Тиковая обработка (проверка обновлений)
            case EVENT_UPDATE: {
                if (_timeoutUpdate.isTimeout()) { // вышел таймаут
                    bool netConnected = MWOSNet::getNetModule()->isConnected(false);
                    if (netConnected && mwos.getCID() > 0) {
                        if (waitUpdate) { // ошибка по таймауту ожидания
                            _lastError = true;
                            onUpdateFinish(0);
                            stepUpdate++;
                        } else { // сейчас не ожидание
                            switch (stepUpdate) {
                                case 0: {
                                    if (_autoUpdate) stepUpdate = 1; // начнем обновление конфигурации по таймауту
                                } break;
                                case 5: {
                                    if (_autoUpdate) {
                                        StartUpdate(); // начнем обновление
                                    } else {
                                        StartRequestFile(5);
                                    }
                                } break;
                                default:
                                    if (stepUpdate > 5) {
                                        stepUpdate = 0;
                                    } else { // запрос очередного файла конфигурации
                                        StartRequestFile(stepUpdate);
                                    }
                            }
                        }
                    } else {
                        _timeoutUpdate.start(20); // ожидаем связи
                    }
                }
            } break;

            // 4. Установка значения (замена onReceiveValue)
            case EVENT_SET_VALUE: {
                if (data.param_id == id_firmware) {
                    int8_t cmd = data.toInt();
                    if (cmd == 1) StartUpdateCFG();
                    if (cmd == 2) StartUpdate();
                }
            } break;

            // 5. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_firmware:
                        data.setValueInt(MWOS_BUILD_TIME);
                        return;
                    case id_autoUpdate:
                        data.setValueInt(_autoUpdate);
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSModule::onEvent(modeEvent, data);
    }

    /**
    * Начать отправку и обновление всех конфигов
    */
    void StartUpdateCFG() {
        #if (LOG_UPDATE > 2)
        MW_LOG_MODULE(this); MW_LOG_LN(F("StartUpdateCFG"));
        #endif
        stepUpdate = 1; // начать обновление файлов конфигурации
        waitUpdate = false;
        _timeoutUpdate.start(10);
    }

    /**
    * Запросить с сайта текстовый файл асинхронно.
    * Если запрашиваемый файл с расширением .cfg и уже есть в первой файловой системе,
    * то он будет отправлен на сервер при запросе.
    * @param fileNum Номер скачиваемого файла [1..5]
    */
    void StartRequestFile(int fileNum) {
        #if (LOG_UPDATE > 2)
        MW_LOG_MODULE(this); MW_LOG(F("StartRequestFile: ")); MW_LOG_LN(fileNum);
        #endif
        if (fileNum < 1 || fileNum > 5) return;
        stepUpdate = fileNum;
        waitUpdate = true;
        _timeoutUpdate.start(100); // 10 сек до таймаута ошибки
        #if (ASYNC_UPDATE > 0)
        xTaskCreate(
            asyncStartRequestFile,   // Функция задачи
            "HTTPS_req",             // Имя задачи
            8192,                    // Размер стека
            this,                    // Параметры
            1,                       // Приоритет
            NULL                     // Сохраняем хэндл задачи
        );
        #else
        AsyncStartRequestFile();
        #endif
    }

    /**
    * Асинхронно запустить апдейт прошивки
    */
    void StartUpdate() {
        #if (LOG_UPDATE > 2)
        MW_LOG_MODULE(this); MW_LOG_LN(F("StartUpdate"));
        #endif
        waitUpdate = true;
        _timeoutUpdate.start(random(23 * 3600 * 15, 25 * 3600 * 15)); // следующий автоапдейт - от 23 до 25 часов
        stepUpdate = 0;
        #if (ASYNC_UPDATE > 0)
        xTaskCreate(
            asyncStartUpdate,        // Функция задачи
            "HTTPS_update",          // Имя задачи
            8192,                    // Размер стека
            this,                    // Параметры
            1,                       // Приоритет
            NULL                     // Сохраняем хэндл задачи
        );
        #else
        AsyncStartUpdate();
        #endif
    }

private:
    #if (ASYNC_UPDATE > 0)
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

    int secureUpdate(const String url) {
        HTTPClient http;
        http.setConnectTimeout(5000); // Тайм-аут соединения 5с
        http.setTimeout(10000);        // Тайм-аут чтения 10с

        if (!http.begin(url)) {
            #if (LOG_UPDATE > 0)
            MW_LOG_MODULE(this); MW_LOG_LN(F("Error URL!"));
            #endif
            return 0;
        }

        int httpCode = http.GET();
        if (httpCode != 200) {
            http.end();
            #if (LOG_UPDATE > 0)
            MW_LOG_MODULE(this); MW_LOG_LN(F("Already the latest version!"));
            #endif
            return 0;
        }

        WiFiClient* stream = http.getStreamPtr();

        // 1. Читаем длину прошивки (4 байта)
        uint32_t fwLength = 0;
        if (stream->readBytes((uint8_t*)&fwLength, 4) != 4) {
            http.end();
            #if (LOG_UPDATE > 0)
            MW_LOG_MODULE(this); MW_LOG_LN(F("Header error!"));
            #endif
            return fwLength;
        }

        // Подготовка Update
        if (!Update.begin(fwLength)) {
            http.end();
            #if (LOG_UPDATE > 0)
            MW_LOG_MODULE(this); MW_LOG(F("Flash error size: ")); MW_LOG_LN(fwLength);
            #endif
            return fwLength;
        }

        wdt_reset();
        onUpdateStart(fwLength);

        // 2. Настройка AES-CTR
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_enc(&aes, (const unsigned char*)AES_KEY, 256);

        uint8_t stream_block[16];
        uint8_t nonce_counter[16];
        size_t nc_off = 0;
        memset(nonce_counter, 0, 16);
        memcpy(nonce_counter, AES_NONCE, 8);
        uint32_t cid = mwos.getCID();
        memcpy(nonce_counter + 8, &cid, 4); // следующие 4 байта - ID устройства

        // 3. Потоковая расшифровка с защитой от зависания
        uint8_t buffer[1024];
        size_t totalRead = 0;
        unsigned long lastDataTime = millis();

        while (totalRead < fwLength) {
            size_t available = stream->available();
            if (available > 0) {
                size_t toRead = min(available, sizeof(buffer));
                toRead = min(toRead, (size_t)(fwLength - totalRead));
                size_t readNow = stream->readBytes(buffer, toRead);

                if (readNow > 0) {
                    mbedtls_aes_crypt_ctr(&aes, readNow, &nc_off, nonce_counter, stream_block, buffer, buffer);
                    Update.write(buffer, readNow);
                    totalRead += readNow;
                    lastDataTime = millis(); // Сбрасываем таймер при получении данных
                    onUpdateProgress(totalRead, fwLength);
                }
            } else {
                // Если данных нет в буфере, ждем до 10 секунд
                if (millis() - lastDataTime > 10000) {
                    #if (LOG_UPDATE > 0)
                    MW_LOG_MODULE(this); MW_LOG_LN(F("OTA Timeout: No data received"));
                    #endif
                    Update.abort();
                    http.end();
                    return fwLength;
                }
                delay(10); // Даем планировщику подышать
            }
            delay(1); // Даем Wi-Fi стеку подышать
            wdt_reset();
        }

        mbedtls_aes_free(&aes);

        // 4. Читаем MD5 (последние 16 байт)
        uint8_t expectedMD5[16];
        if (stream->readBytes(expectedMD5, 16) == 16) {
            char md5Hex[33];
            for (int i = 0; i < 16; i++) {
                sprintf(&md5Hex[i * 2], "%02x", expectedMD5[i]);
            }
            Update.setMD5(md5Hex);
        }

        // 5. Финализация
        if (Update.end(true)) {
            MWOSFS * mwosFS = (MWOSFS *) mwos.FindChildByModuleType(MODULE_FS);
            if (mwosFS) {
                mwosFS->configFileParams(true, "/u" + String(mwos.getCID()) + ".cfg");
            }
            mwos.mwosValue.module_id=id;
            mwos.dispatchEvent(EVENT_POWER_OFF,mwos.mwosValue); // оповестим MWOS о перезагрузке
            nvs_flash_erase(); // почистим настройки
            #if (LOG_UPDATE > 1)
            MW_LOG_MODULE(this); MW_LOG_LN(F("Update OK. Rebooting...")); MW_LOG_FLUSH();
            #endif
            ESP.restart();
        } else {
            #if (LOG_UPDATE > 0)
            MW_LOG_MODULE(this); MW_LOG(Update.errorString()); MW_LOG(F(", ErrorCode: ")); MW_LOG_LN(Update.getError());
            #endif
        }

        http.end();
        return fwLength;
    }

    void AsyncStartUpdate() {
        #if (LOG_UPDATE > 1)
        MW_LOG_MODULE(this); MW_LOG_LN(F("AsyncStartUpdate"));
        #endif
        onUpdateFinish(secureUpdate(getUrl(0)));
        _lastError = true;
        waitUpdate = false;
        _timeoutUpdate.start(random(23 * 3600 * 15, 25 * 3600 * 15)); // следующий автоапдейт - от 23 до 25 часов
    }

    /**
    * Запросить с сайта файл .cfg через firmware.php
    * Результат помещает в файл на первую файловую систему
    */
    bool AsyncStartRequestFile() {
        #if (LOG_UPDATE > 1)
        MW_LOG_MODULE(this); MW_LOG(F("AsyncStartRequestFile: ")); MW_LOG_LN(stepUpdate);
        #endif
        if (stepUpdate == 0) return false;
        if (stepUpdate < 2 || stepUpdate > 4) onUpdateStart(5);

        waitUpdate = true;
        String urlStr = getUrl(stepUpdate);
        MWOSFS * mwosFS = (MWOSFS *) mwos.FindChildByModuleType(MODULE_FS);
        int httpCode = 0;
        String filename = "";

        if (mwosFS != NULL && stepUpdate < 6) {
            filename = mwosFS->getFilenameCfg(stepUpdate - 1);
        }

        WiFiClientSecure client;
        HTTPClient http;

        if (stepUpdate < 5 && filename != "" && mwosFS->fs->exists(filename)) {
            File file = mwosFS->fs->open(filename, MWFILE_READ);
            if (file && !file.isDirectory()) {
                #if (LOG_UPDATE > 1)
                MW_LOG_MODULE(this); MW_LOG(F("FILE: ")); MW_LOG(file.size()); MW_LOG('='); MW_LOG_LN(filename); MW_LOG_FLUSH();
                #endif
                http.begin(urlStr);
                http.addHeader("Content-Type", "text/plain");
                httpCode = http.sendRequest("POST", &file, file.size());
            }
            file.close();
        } else {
            http.begin(urlStr);
            httpCode = http.GET();
        }

        #if (LOG_UPDATE > 1)
        MW_LOG_MODULE(this); MW_LOG(F("httpCode: ")); MW_LOG_LN(httpCode); MW_LOG_FLUSH();
        #endif

        if (httpCode == 200) {
            int totalLength = http.getSize();
            if (filename != "") {
                WiFiClient* stream = http.getStreamPtr();
                File f = mwosFS->fs->open(filename + ".new", MWFILE_WRITE);
                if (f) {
                    uint8_t buffer[512];
                    int remaining = totalLength;

                    while (http.connected() && (remaining > 0 || totalLength == -1)) {
                        size_t size = stream->available();
                        if (size) {
                            int readNow = stream->readBytes(buffer, ((size > sizeof(buffer)) ? sizeof(buffer) : size));
                            f.write(buffer, readNow);
                            if (remaining > 0) remaining -= readNow;
                        }
                        delay(1);
                        wdt_reset();
                    }
                    f.close();

                    // Ротация файлов
                    if (mwosFS->fs->exists(filename)) {
                        mwosFS->fs->remove(filename + ".old");
                        mwosFS->fs->rename(filename, filename + ".old");
                    }
                    mwosFS->fs->rename(filename + ".new", filename);
                }
            }
        }

        http.end();
        _lastError = (httpCode != 200);
        if (stepUpdate < 6) onUpdateProgress(stepUpdate, 5);
        if (stepUpdate > 3) onUpdateFinish(5);
        waitUpdate = false;
        stepUpdate++;
        _timeoutUpdate.start(15);
        return httpCode == 200;
    }

    void onUpdateStart(int maxSize) {
        if (onStart) onStart(0, maxSize);
    }

    void onUpdateFinish(int maxSize) {
        if (onEnd) onEnd(maxSize, maxSize);
    }

    #if (LOG_UPDATE > 4)
    int lastPercent = 0;
    #endif

    void onUpdateProgress(int cur, int total) {
        #if (LOG_UPDATE > 4)
        int percent = cur * 100 / total;
        if (percent < lastPercent || percent > lastPercent + 1 || (percent == 100 && lastPercent != 100)) {
            MW_LOG(percent);
            if (percent > 99) MW_LOG_LN("%");
            else MW_LOG("%..");
            lastPercent = percent;
        }
        #endif
        if (onProgress) onProgress(cur, total);
    }

    String getSite(IPAddress ip) {
        _https = false;
        return String(F("http://")) + ip.toString() + "/mwos3";
    }

    String getSite(String site) {
        _https = false;
        return String(F("http://")) + site;
    }

    String getUrl(int stepA) {
        if (stepA < 0 || stepA > 5) return "";
        String res = getSite(MWOS_SERVER_HOST) + String(F("/firmware.php?v=")) + getVersionBuild() +
                     "&t=" + MWOS_CONTROLLER_TYPE + "&p=" + MWOS_PROJECT +
                     "&b=" + MWOS_BOARD_FULL + "&c=" + String(MWOS_BUILD_TIME) +
                     "&s=" + String(MWOS_USER_HASH, HEX) + "&id=" + String(mwos.getCID());
        if (stepA > 0) res += "&a=" + String(stepA);
        #if (LOG_UPDATE > 1)
        MW_LOG_MODULE(this); MW_LOG(F("POST: ")); MW_LOG_LN(res); MW_LOG_FLUSH();
        #endif
        return res;
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif
#endif