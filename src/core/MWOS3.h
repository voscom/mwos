#ifndef MWOS3_MWOSBASE_H
#define MWOS3_MWOSBASE_H

#ifndef CID
// id контроллера по умолчанию
#define CID 0
#endif

#include "core/adlib/MWArduinoLib.h"
#include "core/adlib/MWWDT.h"
#include "core/adlib/MWStringListBuff.h"
#include "MWOSDebug.h"
#include "MWOSModuleBase.h"
#include "pins/MWOSPinsMain.h"
#include "MWStorage.h"
#include "MWOSUnit.h"
#include "MWOSPins.h"
#include "MWOSParent.h"
#include "storages/MWStorageRAM.h"
#include "storages/MWStorageEEPROM.h"
#include "storages/MWStorageStaticRAM.h"
#include "core/MWOSConsts.h"
#include "core/adlib/MWTimeout.h"

#if defined(ESP32)
#include "storages/MWStorageEspNVS.h"
#include "esp_flash_encrypt.h"
#ifndef MWOS_CID_NVS
#define MWOS_CID_NVS "nvs"
#endif
#endif


#ifndef MWOS_DEBUG_BOUDRATE
#define MWOS_DEBUG_BOUDRATE 115200
#endif

#ifndef MWOS_PARAM_STORAGES_COUNT
#if defined(ESP32) || defined(ESP8266)
#ifndef MWOSStorageStaticRAM_NO
// максимальное количество хранилищ (на контроллерах с поддержкой StaticRAM)
#define MWOS_PARAM_STORAGES_COUNT 3
#else
// максимальное количество хранилищ (на контроллерах без поддержки StaticRAM)
#define MWOS_PARAM_STORAGES_COUNT 2
#endif
#else
#ifndef MWOSStorageStaticRAM_NO
// максимальное количество хранилищ (на контроллерах с поддержкой StaticRAM)
#define MWOS_PARAM_STORAGES_COUNT 2
#else
// максимальное количество хранилищ (на контроллерах без поддержки StaticRAM)
#define MWOS_PARAM_STORAGES_COUNT 1
#endif
#endif
#endif

class MWOS3 : public MWOSParent {
public:

    MWStorage * storages[MWOS_PARAM_STORAGES_COUNT];

    // ID этого контроллера
    uint32_t cid=0;

    /**
     * Общее количество параметров всех модулей в системе
     */
    uint32_t paramsCount;

    /**
     * Общее количество всех значений всех параметров всех модулей в системе
     */
    uint32_t totalParamsValuesCount=0;

    /***
     * Основной блок портов микроконтроллера (обычно порты 0-127)
     * Платы расширения добавлять так: mwos.pins.add(new MWOSPins());
     */
    MWOSPinsMain pins;

    // модуль, вызвавший событие
    MWOSModuleBase * eventModule;
    // параметр, вызвавший событие
    MWOSParam * eventParam;
    // индекс массива параметра, вызвавший событие
    uint16_t eventArrayIndex;
    MWValue mwosValue;

     // общее количество модулей, включая модуль связи и модуль времени
    uint16_t modulesCount=0;

    MWOS3() : MWOSParent((char *) F("MWOS3")) {
        unitType=OS;
    }

    void AddStorage(MWStorage * newStorage) {
        if (newStorage->_num>=MWOS_PARAM_STORAGES_COUNT) return;
        storages[newStorage->_num]=newStorage;
        MW_LOG_TIME(); MW_LOG(F("AddStorage: ")); MW_LOG_PROGMEM(newStorage->name); MW_LOG('='); MW_LOG_LN(newStorage->_num);
    }

    void start() {
#if PAUSE>0
        delay(PAUSE);
#endif
        //MW_DEBUG_BEGIN(MWOS_DEBUG_BOUDRATE);
        getCID();
        MW_LOG(F("\n\nMWOS3 start: ")); MW_LOG(cid); MW_LOG('-'); MW_LOG(getLID());
        MW_LOG(F(". Free mem: ")); MW_LOG_LN(getFreeMemory());
#ifdef ESP32
#if DEBUG>0
#else
        esp_log_level_set("*", ESP_LOG_NONE);  // Отключить все теги логов ESP
#endif
        MW_LOG(F("Total heap: ")); MW_LOG(ESP.getHeapSize()); MW_LOG(F(", Free: ")); MW_LOG_LN(ESP.getFreeHeap());
        MW_LOG(F("Total PSRAM: ")); MW_LOG(ESP.getPsramSize()); MW_LOG(F(", Free: ")); MW_LOG_LN(ESP.getFreePsram());
        switch (esp_reset_reason()) {
            case ESP_RST_UNKNOWN:
                MW_LOG_LN(F("Reset reason can not be determined"));
                break;
            case ESP_RST_POWERON:
                MW_LOG_LN(F("Reset due to power-on event"));
                break;
            case ESP_RST_EXT:
                MW_LOG_LN(F("Reset by external pin (not applicable for ESP32)"));
                break;
            case ESP_RST_SW:
                MW_LOG_LN(F("Software reset via esp_restart"));
                break;
            case ESP_RST_PANIC:
                MW_LOG_LN(F("Software reset due to exception/panic"));
                break;
            case ESP_RST_INT_WDT:
                MW_LOG_LN(F("Reset (software or hardware) due to interrupt watchdog"));
                break;
            case ESP_RST_TASK_WDT:
                MW_LOG_LN(F("Reset due to task watchdog"));
                break;
            case ESP_RST_WDT:
                MW_LOG_LN(F("Reset due to other watchdogs"));
                break;
            case ESP_RST_DEEPSLEEP:
                MW_LOG_LN(F("Reset after exiting deep sleep mode"));
                break;
            case ESP_RST_BROWNOUT:
                MW_LOG_LN(F("Brownout reset (software or hardware)"));
                break;
            case ESP_RST_SDIO:
                MW_LOG_LN(F("Reset over SDIO"));
                break;
            default:
                MW_LOG_LN(F("Reset reason: unknown code"));
                break;
        }
        if (esp_flash_encryption_enabled()) {
            MW_LOG_LN(F("Flash encryption is ENABLED"));
        } else {
            MW_LOG_LN(F("Flash encryption is DISABLED"));
        }
        MW_LOG(F("Chip ID: ")); MW_LOG_LN(getChipID());
        MW_LOG_FLUSH();
#endif
        MW_LOG_LN();
#if MWOS_LOG_MODULES>0
        MW_LOG_LN(F("Modules list:"));
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext!=nullptr && moduleNext->unitType==MODULE) {
            MW_LOG_MODULE(moduleNext); MW_LOG(F("- params: ")); MW_LOG_LN(moduleNext->GetParamCount(false)); MW_LOG_FLUSH();
#if MWOS_LOG_MODULES>0
            MWOSParam * param=(MWOSParam *) moduleNext->child;
            while (param!=nullptr && param->unitType==PARAM) {
                MW_LOG_MODULE(moduleNext,param,param->arrayCount()); MW_LOG(F("- values: ")); MW_LOG_LN(param->arrayCount()); MW_LOG_FLUSH();
                param=(MWOSParam *) param->next;
            }
#endif
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
#endif
        MW_LOG_LN();
        MW_LOG(F("Compile time: ")); MW_LOG_LN((uint64_t ) MWOS_BUILD_TIME);
        MW_LOG(F("LIBS: ")); MW_LOG_PROGMEM((char *) &mwos_libs); MW_LOG_LN();
        MW_LOG_FLUSH();
        // добавим хранилища (если не задано раньше)
#ifndef MWOS_AUTO_STORAGES_NO
#if !defined(MWOSStorageEEPROM_NO) && !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
        if (MWStorage::storagesCount == 0) AddStorage(new MWStorageEEPROM()); // если не задано бинарное хранилище, то создадим хранилище в EEPROM
#endif
#ifndef MWOSStorageStaticRAM_NO
        if (MWStorage::storagesCount == 1) AddStorage(new MWStorageStaticRAM()); // если задано только бинарное хранилище EEPROM, то создадим хранилище в StaticRAM (если платформа поддерживает)
#endif
#if defined(ESP32) && !defined(MWOSStorageNVS_NO)
        if (MWStorage::storagesCount == 2) AddStorage(new MWStorageEspNVS()); // если не задано бинарное хранилище NVS, то создадим хранилище в NVS
#endif
#endif
        // подсчитаем общее количество параметров
        wdt_reset();
        paramsCount=getParamsCount();
        totalParamsValuesCount=getValuesCount();
        randomSeed(micros());
        MW_LOG_TIME(); MW_LOG(F("Storages count: "));
        if (MWStorage::storagesCount>MWOS_PARAM_STORAGES_COUNT) {
            MW_LOG(F(" ERROR: ")); MW_LOG((int) MWStorage::storagesCount);  MW_LOG('/');
            MWStorage::storagesCount=MWOS_PARAM_STORAGES_COUNT;
        }
        MW_LOG_LN((int) MWStorage::storagesCount);
        // создадим хранилища параметров
        uint16_t crc8x8=modulesCRC16(0);
        for (uint8_t st = 0; st < MWStorage::storagesCount; st++) {
            wdt_reset();
            storages[st]->onInit(StorageBitSize(st), crc8x8); // настроим хранилище
        }
        // вызовем инициализацию модулей
        mwosValue.module_id=0xffff;
        mwosValue.param_index=0;
        MW_LOG_TIME(); MW_LOG_LN(F("EVENT_INIT"));
        dispatchEvent(EVENT_INIT,mwosValue); // инициализация всех модулей
        for (uint8_t st = 0; st < MWStorage::storagesCount; st++) {
            wdt_reset();
            if (initStorage(st, crc8x8)) { // инициализация всех хранилищ
                mwosValue.param_index |= 1 << st; // битовая маска неинициализированных хранилищ
            }
        }
        if (mwosValue.param_index>0) {
            MW_LOG_TIME(); MW_LOG_LN(F("EVENT_EMPTY_STORAGES"));
            // вызовем событие EVENT_EMPTY_STORAGES.
            // по этому событию модули файловой системы загружают значения параметров из файлов конфигурации.
            // если загруженные параметры из неинициализированных хранилищ, то этим параметрам вызывается событие EVENT_SET_VALUE.
            // которое сохраняет значение в хранилище и вызывает событие EVENT_CHANGE модулю этого параметра.
            dispatchEvent(EVENT_EMPTY_STORAGES,mwosValue);

        }
        wdt_reset();
        MW_LOG_TIME(); MW_LOG(F("MWOS INITED! Modules: ")); MW_LOG(modulesCount); MW_LOG(F(", Params: ")); MW_LOG(paramsCount); MW_LOG(F(", Values: ")); MW_LOG(totalParamsValuesCount); MW_LOG(F(", Free mem: ")); MW_LOG_LN(getFreeMemory()); MW_LOG_LN();
        MW_LOG_LN();
    }

    uint32_t getValuesCount() {
        uint32_t res=0;
        MWOSModuleBase *moduleNext = (MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType == UnitType::MODULE) {
            res+=moduleNext->GetParamCount(true);
            moduleNext = (MWOSModuleBase *) moduleNext->next;
        }
        return res;
    }

    uint32_t getParamsCount() {
        uint32_t res=0;
        MWOSModuleBase *moduleNext = (MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType == UnitType::MODULE) {
            res+=moduleNext->GetParamCount(false);
            moduleNext = (MWOSModuleBase *) moduleNext->next;
        }
        return res;
    }

    uint32_t getModulesCount() {
        uint32_t res=0;
        MWOSModuleBase *moduleNext = (MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType == UnitType::MODULE) {
            res++;
            moduleNext = (MWOSModuleBase *) moduleNext->next;
        }
        return res;
    }

    /**
     * Вызвать событие для всех модулей
     * @param modeEvent Тип события
     */
    void dispatchEvent(MWOSModeEvent modeEvent, MWValue &data) {
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
#ifdef MWOS_PROFILER_MS
        MW_DEBUG_UPDATE_FPS();
        MWTimeout<uint16_t,1000> profilerTm;
#endif
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            wdt_reset();
#ifdef MWOS_PROFILER_MS
            profilerTm.start();
#endif
#ifdef MWOS_PROFILER_MEM
            uint32_t freeMem=getFreeMemory();
#endif
            moduleNext->onEvent(modeEvent,data);
#ifdef MWOS_PROFILER_MS
            if (profilerTm.isTimeout(MWOS_PROFILER_MS)) {
                MW_LOG_MODULE(moduleNext); MW_LOG(':');  MW_LOG((int8_t) modeEvent); MW_LOG(F(" ALERT!!! time ms: ")); MW_LOG_LN(profilerTm.timeout());
            }
#endif
#ifdef MWOS_PROFILER_MEM
            int32_t nowMem=getFreeMemory();
            int32_t dMem=freeMem-nowMem;
            if (dMem>MWOS_PROFILER_MEM) {
                MW_LOG_MODULE(moduleNext); MW_LOG(':');  MW_LOG((int8_t) modeEvent); MW_LOG(F(" WARNING!!! mem lost: ")); MW_LOG(dMem); MW_LOG(F(", free: ")); MW_LOG_LN(nowMem);
            }
#endif
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        wdt_reset();
        if (modeEvent==EVENT_POWER_OFF) { // перед выключением
            saveChangedParams(); // сохраним признаки несохраненных параметров при включении питания
        }
        for (uint8_t storageType = 0; storageType < MWStorage::storagesCount; storageType++) {
            wdt_reset();
            storages[storageType]->commit(); // произвести комиты в хранилище, если были изменения
        }
    }

    void update() {
        dispatchEvent(EVENT_UPDATE,mwosValue);
    }

    /***
     * Расчитать контрольную сумму CRC16 по именам и количествам параметров всех модулей (для актуальности хранилищ)
     * @return  CRC16
     */
    uint16_t modulesCRC16(uint8_t storageType) {
        if (storageType >= MWStorage::storagesCount) return 0;
        return MWOS_BUILD_TIME & 0xffff;
        /*
        MW_CRC16 crc16obj;
        crc16obj.start();
        if (crcOfChilds((MW_CRC *) &crc16obj, storageType)) id16bit= true; // crc для модулей и их параметров (заодно, проверим есть ли id>255)
        // занимаемое хранилищем место
        uint16_t crc16=(uint16_t) crc16obj.getCRC();
        MW_LOG_TIME(); MW_LOG(F("Storage ")); MW_LOG(storageType); MW_LOG(F(" CRC16=")); MW_LOG_LN(crc16,HEX);
        return crc16;
        */
    }

    /**
     * Получить базовое смещение в хранилище
     * @param storageType
     * @return
     */
    uint32_t storageOffset(uint8_t storageType) {
        if (storageType >= MWStorage::storagesCount) return 0;
        uint32_t totalBitSize=0;
        if (storageType == MWOS_STORAGE_LOG_OPTIONS) { // в этом хранилище маска признаков записи в журнал для всех параметров
            totalBitSize=paramsCount; // зарезервируем место под признаки записи в журнал
            if ((totalBitSize & 7) > 0) totalBitSize += 8-(totalBitSize & 7); // выравнивание на байт
            totalBitSize+=paramsCount; // зарезервируем место под признаки, что параметр изменен
            if ((totalBitSize & 7) > 0) totalBitSize += 8-(totalBitSize & 7); // выравнивание на байт
        }
        return totalBitSize;
    }

    uint32_t StorageBitSize(uint8_t storageType) {
        if (storageType >= MWStorage::storagesCount) return 0;
        uint32_t totalBitSize=storageOffset(storageType);
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        uint32_t bitPartBitsSize=0;
        uint32_t bytePartBitsSize=0;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            bitPartBitsSize+=moduleNext->paramsBitsSize(storageType, true); // битовые параметры
            bytePartBitsSize+=moduleNext->paramsBitsSize(storageType, false); // байтовые параметры
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        if ((bitPartBitsSize & 7) > 0) bitPartBitsSize += 8-(bitPartBitsSize & 7); // битовое хранилище дополним до байта
        storages[storageType]->_offsetBitsByteSide=bitPartBitsSize; // смещение байтового хранилища, после битового (в байтах)
        totalBitSize+=bytePartBitsSize+bitPartBitsSize+ (storages[storageType]->_checkSumSize << 3); // добавим размер контрольной суммы вначале хранилища
#if LOG_STORAGE>=0
        MW_LOG_TIME(); MW_LOG_PROGMEM(storages[storageType]->name); MW_LOG(F(" = PartBits: ")); MW_LOG(bitPartBitsSize); MW_LOG(F(", PartBytes: ")); MW_LOG( bytePartBitsSize); MW_LOG(F(", total: ")); MW_LOG_LN( totalBitSize);
#endif
        return totalBitSize; // добавим размер контрольной суммы вначале хранилища
    }

    /**
     * Искать ранее не сохраненные при инициализации (module.onInit) параметры и сохранить их module.getValue
     * @param storageType
     */
    void SaveNotSavedParams(uint8_t storageType) {
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            MWOSParam * paramNow=(MWOSParam *) moduleNext->child;
            while (paramNow && paramNow->unitType==UnitType::PARAM) {
                MW_LOG_MODULE(moduleNext,paramNow,paramNow->arrayCount()); MW_LOG(F("Param storage: ")); MW_LOG(storageType); MW_LOG('='); MW_LOG_LN(paramNow->saved);
                if (paramNow->IsStorage(storageType) && !paramNow->saved) { // этот параметр не сохранен в хранилище
                    MW_LOG_MODULE(moduleNext,paramNow,paramNow->arrayCount()); MW_LOG_LN(F("default value not found!"));
                    mwosValue.module_id=moduleNext->id;
                    mwosValue.param_id=paramNow->id;
                    if (paramNow->IsString()) { // строки запрашиваем целиком
                        mwosValue.param_index=0xffff;
                        moduleNext->getValue(mwosValue,paramNow);
                        saveValueToStorage(mwosValue);
                    } else
                    for (uint16_t ind = 0; ind < paramNow->arrayCount(); ++ind) {
                        mwosValue.param_index=ind;
                        moduleNext->getValue(mwosValue,paramNow);
                        saveValueToStorage(mwosValue);
                    }
                }
                paramNow=(MWOSParam *) paramNow->next;
            }
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
    }

    /***
     * Инициализировать хранилище, записав контрольную сумму и выставить признак, что хранилище актуально
     * @param storageType
     * @param crc16modules
     * @return Хранилище не было инициализировано ранее
     */
    bool initStorage(uint8_t storageType, uint16_t crc16modules) {
        if (storageType >= MWStorage::storagesCount) return false;
        MW_LOG_TIME(); MW_LOG(F("initStorage ")); MW_LOG_LN(storageType);
        if (IsStorageInited(storageType)) { // у хранилища успешно совпала контрольная сумма
            if (storageType == MWOS_STORAGE_LOG_OPTIONS) { // в этом хранилище маска признаков записи в журнал для всех параметров
                loadLogOptions(); // если хранилище актуально - загрузим признаки из него
            }
            return false;
        }
        // контрольная сумма не совпала - это новое хранилище
        if (storageType == MWOS_STORAGE_LOG_OPTIONS) { // в этом хранилище маска признаков записи в журнал для всех параметров
            MW_LOG_TIME(); MW_LOG(F("saveLogOptions ")); MW_LOG_LN(storageType);
            SetChangedAll(true, false); // раз только создали основное хранилище - пометим все параметры как измененные
            saveLogOptions(); // сохраним признаки по умолчанию в хранилище
        }
        MW_LOG_TIME(); MW_LOG(F("SaveNotSavedParams ")); MW_LOG_LN(storageType);
        SaveNotSavedParams(storageType);
        storages[storageType]->initStorage(crc16modules);
        storages[storageType]->commit(true);
        return true;
    }

    /***
     * Проверить, было ли инициализировано хранилище
     * @param storageType
     * @return
     */
    bool IsStorageInited(uint8_t storageType) {
        if (storageType >= MWStorage::storagesCount) return false;
        return storages[storageType]->_inited;
    }

    /**
     * Найти модуль по id
     * @param module_id
     * @return
     */
    MWOSModuleBase * getModule(uint16_t module_id) {
        return (MWOSModuleBase *) FindChildById(module_id);
    }

    /**
     * Найти блок портов по номеру PIN
     * @param pinNum Номер PIN
     * @return  Плата порта или nullptr
     */
    MWOSPins * getPin(MWOS_PIN_INT pinNum) {
        if (pinNum>=0) {
            MWOSPins * res=&pins;
            while (res) {
                if (res->isPin(pinNum)) return res;
                res=res->next;
            }
        }
        return nullptr; // если не нашли порт - то пустой класс
    }

    /**
     * Задать режим на вход или на выход
     * и подтяжку, если нужно
     * @param pinNum    Номер порта
     * @param	mode	режим
     */
    bool mode(MWOS_PIN_INT pinNum, MW_PIN_MODE mode) {
        if (pinNum<0) return false;
        MWOSPins * _pin=getPin(pinNum);
        if (!_pin) return false;
        return _pin->mode(mode);
    }

    /***
     * Записать бинарное значение в порт
     * @param pinNum    Номер порта
     * @param value     бинарное значение
     * @return  Успешно
     */
    bool writeValueDigital(MWOS_PIN_INT pinNum, bool value) {
        if (pinNum<0) return false;
        MWOSPins * _pin=getPin(pinNum);
        if (!_pin) return false;
        //MW_LOG_TIME(); MW_LOG(F("dig pin")); MW_LOG(pinNum); MW_LOG('='); MW_LOG_LN(value);
        return _pin->write(value);
    }

    /***
     * Записать значение в порт ШИМ
     * @param pinNum    Номер порта
     * @param value     Аналоговое значение
     * @return  Успешно
     */
    bool writeValuePWM(MWOS_PIN_INT pinNum, uint16_t value) {
        if (pinNum<0) return false;
        MWOSPins * _pin=getPin(pinNum);
        if (!_pin) return false;
        MW_LOG_TIME(); MW_LOG(F("pwm pin")); MW_LOG(pinNum); MW_LOG('='); MW_LOG_LN(value);
        return _pin->writePWM(value);
    }

    /***
     * Записать аналоговое значение в ЦАП
     * @param pinNum    Номер порта
     * @param value     Аналоговое значение
     * @return  Успешно
     */
    bool writeValueDAC(MWOS_PIN_INT pinNum, uint16_t value) {
        if (pinNum<0) return false;
        MWOSPins * _pin=getPin(pinNum);
        if (!_pin) return false;
        MW_LOG_TIME(); MW_LOG(F("dac pin")); MW_LOG(pinNum); MW_LOG('='); MW_LOG_LN(value);
        return _pin->writeDAC(value);
    }

    /***
     * Прочитать аналоговое значение из порта
     * @param pinNum    Номер порта
     * @return  аналоговое значение
     */
    uint16_t readValueAnalog(MWOS_PIN_INT pinNum) {
        if (pinNum<0) return 0;
        MWOSPins * _pin=getPin(pinNum);
        if (!_pin) return 0;
        return _pin->readADC();
    }

    /***
     * Прочитать бинарное значение из порта
     * @param pinNum    Номер порта
     * @return  бинарное значение
     */
    bool readValueDigital(MWOS_PIN_INT pinNum) {
        if (pinNum<0) return false;
        MWOSPins * _pin=getPin(pinNum);
        if (!_pin) return false;
        return _pin->read();
    }

    /**
     * Срочно все сохранить и перевести процессор в режим сна.
     * Будить уровнем на заданной ножке (можно не задавать).
     * Перед сном, ожидает изменения уровня сигнала на заданной ножке, что-бы сразу не просыпаться.
     * @param pinWakeup Ножка процессора, уровнем на которой будить процессор
     * @param signalLevel   Уровень, которым будить процессор LOW или HIGH
     */
    void Sleep(int16_t pinWakeup=-1, uint8_t signalLevel=1) { // срочно все сохранить и перевести процессор в режим глубокого сна
        dispatchEvent(EVENT_POWER_OFF,mwosValue); // вызвать всем модулям событие сохранения перед выключением
        if (pinWakeup>=0) {
            pinMode(pinWakeup,INPUT);
            delay(50);
            while (digitalRead(pinWakeup)==signalLevel) delay(50); // ждем, пока не отпустят кнопку
        }
#ifdef ESP32
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
        if (pinWakeup>=0) {
            uint32_t mask= 1 << pinWakeup;
            esp_deep_sleep_enable_gpio_wakeup(mask,(esp_deepsleep_gpio_wake_up_mode_t) signalLevel);
        }
        esp_deep_sleep_start();
#elif SOC_PM_SUPPORT_EXT_WAKEUP
        if (pinWakeup>=0) {
            esp_sleep_enable_ext0_wakeup((gpio_num_t) pinWakeup, signalLevel);
        }
        esp_deep_sleep_start();
#endif // SOC
#endif // ESP32
    }

    /***
     * Рассчитать битовое смещение модуля и параметра для этого типа хранилища. Отдельно для битовых и байтовых параметров.
     * @param module
     * @param param
     * @param arrayIndex
     * @return  Битовое смещение от начала хранилища
     */
   uint32_t getBitOffset(MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex) { // вызывается из модуля
        int8_t storageType=param->getStorage();
        uint32_t bitOffset=storageOffset(storageType);
        bool IsParamBit=param->IsBit(); // это байтовое хранилище (false - битовое)
        if (!IsParamBit) {
            bitOffset+=storages[storageType]->_offsetBitsByteSide; // учтем смещение начала байтового хранилища, относительно начала хранилища (до этого идет битовое хранилище)
#if LOG_STORAGE>1
            MW_LOG_MODULE(module,param,arrayIndex); MW_LOG('+'); MW_LOG(storages[storageType]->_offsetBitsByteSide); MW_LOG('='); MW_LOG_LN(bitOffset);
#endif
        }
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            if (moduleNext==module) {
                bitOffset += moduleNext->getParamBitsOffset(param); // добавим смещение параметра внутри модуля
                if (arrayIndex>0) {
                    int16_t bitSize=param->bitsSize(false);
                    bitOffset+=arrayIndex*bitSize; // добавим смещение массива внутри параметра
                }
                return bitOffset;
            }
            bitOffset+=moduleNext->paramsBitsSize(storageType,IsParamBit);
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return 0;
    }

    mwos_size saveValueToStorage(MWValue &data) {
        if (data.module_id==0xffff) return 0;
        MWOSModuleBase * module=getModule(data.module_id);
        if (!module) return 0;
        MWOSParam * param=module->getParam(data.param_id);
        if (!param) return 0;
        if (data.size<1 || !data.getBytes()) data.start(8,true);
        return saveValueToStorage(data.getBytes(),module,param,data.param_index,data.size);
    }

    mwos_size saveValueToStorage(uint8_t * buff,MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex, int16_t buffSize) { // вызывается из модуля и из контроллера
        if (!buff || !module || !param || !buffSize || arrayIndex>=param->arrayCount()) return 0;
        if (param->IsString()) arrayIndex=0;
        if (arrayIndex>=param->arrayCount()) return 0;
        uint8_t storageType=param->getStorage();
        if (storageType < MWStorage::storagesCount) {
            uint32_t bitOffset=getBitOffset(module,param,arrayIndex); // найдем смещение в хранилище
            int16_t bitSize=param->bitsSize(false);
            uint16_t maxSize=param->byteSize(false);
            if (param->IsString()) {
                bitSize = param->bitsSize(true);
                if (storageType==MWOS_STORAGE_NVS) maxSize=0;
                else maxSize=param->byteSize(true);
            }
            if (maxSize>buffSize) maxSize=buffSize;
            mwos_size res=storages[storageType]->saveValue(buff,bitOffset,bitSize,maxSize);
#ifdef LOG_STORAGE
            MW_LOG_MODULE(module,param,arrayIndex); MW_LOG(F("Storage ")); MW_LOG(storageType); MW_LOG(F(" save bytes ")); MW_LOG(bitOffset); MW_LOG('='); MW_LOG(bitSize); MW_LOG(':'); MW_LOG(res);
            if (res>0) {
                MW_LOG('='); MW_LOG_BYTES(buff,res);
            }
            MW_LOG_LN();
#endif
            return res;
        } else {
            //MW_LOG_MODULE(module,param,arrayIndex); MW_LOG(F("Error save storage: ")); MW_LOG(storageType); MW_LOG('/'); MW_LOG_LN(MWOSStorage::storagesCount);
        }
        return 0;
    }

    /**
     * Считать данные из хранилища. Для строк и массива - если приемный буфер не задан в data, то создает промежуточный.
     * @param data  Адресат (модуль, параметр, индекс) и место для данных.
     * @return Сколько бит считано
     */
    mwos_size loadValueFromStorage(MWValue &data) {
        if (data.module_id==0xffff) return 0;
        MWOSModuleBase * module=getModule(data.module_id);
        if (!module) return 0;
        MWOSParam * param=module->getParam(data.param_id);
        if (!param) return 0;
        MW_LOG_MODULE(module, param, data.param_index); MW_LOG(F("loadData = ")); MW_LOG_LN(data.toString());
        mwos_size res = 0;
        data.status.error=true; // по умолчанию это ошибка, потом ее сбросит, если все норм
        if (param->IsString()) { // это строка целиком
            uint16_t len=param->byteSize(true);
            uint8_t * buffer = nullptr;
            if (data.status.autoMemory) {
                buffer = new (std::nothrow) uint8_t[len];
            } else { // в data уже принят приемный буфер
                buffer = data.getBytes();
            }
            if (buffer) {
                data.status.error=false;
                res = loadValueFromStorage(buffer,module,param,data.param_index,len);
                if (res>0 && data.status.autoMemory) { // если ранее задали буфер вручную, то все уже скопировано туда
                    if (res>0 && data.param_index < param->arrayCount()) { // это конкретный параметр строки
                        data.setString((const char *) getSubString(buffer,data.param_index,';'));
                    } else {
                        data.setString((const char *) buffer);
                    }
                    delete[] buffer;
                }
            }
        } else { // это одиночный параметр
            if (data.param_index<param->arrayCount()) {
                uint16_t len=param->byteSize(false);
                if (data.size<len) data.start(len,true);
                res = loadValueFromStorage(data.getBytes(),module,param,data.param_index,len);
                data.status.error=false;
            }
        }
        if (res==0 && !data.isEmpty() && !param->saved) {
            saveValueToStorage(data.getBytes(),module,param,data.param_index,data.size); // запишем в хранилище
            param->saved=true;
        }
        return res;
    }

    mwos_size loadValueFromStorage(uint8_t * buff, MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex, int16_t buffSize) { // вызывается из модуля и из контроллера
        if (!buff || !module || !param || !buffSize || arrayIndex>=param->arrayCount()) return 0;
        if (param->IsString()) arrayIndex=0;
        if (arrayIndex>=param->arrayCount()) return 0;
        int8_t storageType=param->getStorage();
        if (IsStorageInited(storageType) && storageType < MWStorage::storagesCount) {
            int16_t bitSize;
            if (param->IsString()) bitSize=param->bitsSize(true);
            else bitSize=param->bitsSize(false);
            int16_t bitOffset=getBitOffset(module,param,arrayIndex); // найдем смещение в хранилище
            if (bitSize > (buffSize << 3)) bitSize = buffSize << 3;
            int16_t res=storages[storageType]->loadValue(buff,bitOffset,bitSize);
#if LOG_STORAGE>1
            MW_LOG_MODULE(module,param,arrayIndex); MW_LOG(F("Storage ")); MW_LOG(storageType); MW_LOG(F(" load bytes ")); MW_LOG(bitOffset); MW_LOG('='); MW_LOG(bitSize); MW_LOG(':'); MW_LOG(res);
            if (res>0) {
                MW_LOG('='); MW_LOG_BYTES(buff,res);
            }
            MW_LOG_LN();
#endif
            return res;
        } else {
#if LOG_STORAGE>0
            MW_LOG_MODULE(module,param,arrayIndex); MW_LOG(F("Error load storage: ")); MW_LOG_LN(storageType);
#endif
        }
        return 0;
    }

    /**
     * Сохранить признаки записи в журнал для всех параметров
     */
    void saveLogOptions() {
        MWBitsMask logMask;
        logMask.setSize(paramsCount);
        int32_t numParam=0;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            MWOSParam * paramNext=(MWOSParam *) moduleNext->child;
            while (paramNext && paramNext->unitType==UnitType::PARAM) {
                // MW_LOG_MODULE(moduleNext,paramNext); MW_LOG_LN(F("logMask.setBit"));
                logMask.setBit(paramNext->saveToLog,numParam++);
                paramNext=(MWOSParam *) paramNext->next;
            }
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        MW_LOG_TIME(); MW_LOG_LN(F("saveLogOptions..."));
        mwos_size res=storages[MWOS_STORAGE_LOG_OPTIONS]->saveValue(logMask.bits, 0, paramsCount, logMask.bytesSize());
        MW_LOG_TIME(); MW_LOG(F("Storage log options save ")); MW_LOG(MWOS_STORAGE_LOG_OPTIONS); MW_LOG('='); MW_LOG_LN(res);
        logMask.free();
    }

    /**
     * Сохранить признаки измененных параметров
     */
    void saveChangedParams() {
        SetChangedAll(2,false); // признак sended тоже надо сохранить
        MWBitsMask changedMask;
        changedMask.setSize(paramsCount);
        int32_t numParam=0;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            MWOSParam * paramNext=(MWOSParam *) moduleNext->child;
            while (paramNext && paramNext->unitType==UnitType::PARAM) {
                changedMask.setBit(paramNext->changed,numParam++);
                paramNext=(MWOSParam *) paramNext->next;
            }
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        MW_LOG_TIME(); MW_LOG_LN(F("saveChangedParams..."));
        uint32_t offset=paramsCount;
        if ((offset & 7) > 0) offset += 8-(offset & 7); // выравнивание на байт
        mwos_size res=storages[MWOS_STORAGE_LOG_OPTIONS]->saveValue(changedMask.bits, offset, paramsCount, changedMask.bytesSize());
        MW_LOG_TIME(); MW_LOG(F("ChangedParams save ")); MW_LOG(MWOS_STORAGE_LOG_OPTIONS); MW_LOG('='); MW_LOG_LN(res);
        changedMask.free();
    }

    /**
     * Загрузить признаки записи в журнал для всех параметров
     */
    void loadLogOptions() {
        MWBitsMask logMask;
        logMask.setSize(paramsCount);
        MWBitsMask changedMask;
        changedMask.setSize(paramsCount);
        mwos_size res=storages[MWOS_STORAGE_LOG_OPTIONS]->loadValue(logMask.bits, 0, paramsCount); // признаки сохранения в журнал
        uint32_t offset=paramsCount;
        if ((offset & 7) > 0) offset += 8-(offset & 7); // выравнивание на байт
        storages[MWOS_STORAGE_LOG_OPTIONS]->loadValue(changedMask.bits, offset, paramsCount); // признаки измененных параметров
        int32_t numParam=0;
        if (res>=logMask.bytesSize()) {
            MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
            while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
                MWOSParam * paramNext=(MWOSParam *) moduleNext->child;
                while (paramNext && paramNext->unitType==UnitType::PARAM) {
                    paramNext->saveToLog=logMask.getBit(numParam);
                    paramNext->changed=changedMask.getBit(numParam);
                    numParam++;
                    paramNext=(MWOSParam *) paramNext->next;
                }
                moduleNext=(MWOSModuleBase *) moduleNext->next;
            }
        }
        MW_LOG_TIME(); MW_LOG(F("Storage log options load ")); MW_LOG(MWOS_STORAGE_LOG_OPTIONS); MW_LOG('='); MW_LOG_LN(res);
        logMask.free();
        changedMask.free();
    }

    /***
     * Событие изменения значения для записи в журнал
     * ищет все модули журнала и генерирует для них onReceiveValue с командой mwos_server_cmd_param_event
     *
     * можно вызывать из пользовательских модулей
     * или вызывается автоматически из MWOSNetTransmitter, если у параметра установлен saveToLog
     * значения таких параметров будут сохранены в журнал, вместо отправки
     * если в ОС нет модулей журналов, то только тогда отправляется напрямую
     * @param module
     * @param param
     * @param arrayIndex    Индекс значения (0xffff - все значения параметра)
     * @return  Количество дополненных журналов (обычно - 1). Если в ОС нет модулей журналов, то вернет - 0
     */
    int toLog(MWOSModuleBase * module, MWOSParam * param, uint16_t arrayIndex=0xffff) {
        int res= 0;
        if (arrayIndex!=0xffff && param->indexCount()>=arrayIndex) return res; // ошибочный индекс
        MWValue fields;
        fields.module_id=module->id;
        fields.param_id=param->id;
        fields.param_index=arrayIndex;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            if (moduleNext->moduleType==ModuleType::MODULE_LOG) {
                if (arrayIndex==0xffff) {
                    for (int16_t i = 0; i < param->indexCount(); i++) {
                        fields.param_index=i;
                        moduleNext->onEvent(EVENT_LOG_VALUE,fields); // для каждого индекса вызовем журналу событие прихода для журнала
                    }
                } else
                    moduleNext->onEvent(EVENT_LOG_VALUE,fields); // для указанного индекса вызовем журналу событие прихода для журнала
                res++;
            }
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        module->SetParamChanged(param,arrayIndex, false); // сбросим флаг отправки
        return res;
    }

    /**
     * Установить (или снять) признак изменения всех параметров всех модулей
     * Обычно используется для отправки всех значений после соединения
     * @param changed   0=снять, 1=Установить, 2=установить для всех, у кого установлено sended или changed
     * @param onlyControl  Только для параметров управления
     * @return  Количество измененных параметров
     */
    uint16_t SetChangedAll(uint8_t changed, bool onlyControl) {
        uint16_t res=0;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            res+=moduleNext->SetParamChangedByParamId(UINT16_MAX,UINT16_MAX,changed,onlyControl);
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return res;
    }

    /**
     * Установить (или снять) признак отправки информации всех параметров всех модулей
     * @param needSendInf   +1 (bit0)  - всем параметрам всех модулей, +2 (bit1) - всем модулям
     * @return  Сколько установили
     */
    uint16_t SetSendInfAll(uint8_t needSendInf) {
        uint16_t res=0;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            if ((needSendInf & 2)>0) moduleNext->sendInf=true;
            if ((needSendInf & 1)>0) res+=moduleNext->SetParamSendInfByParamId(true, UINT16_MAX);
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return res;
    }

    /***
     * Ищет параметр, где используется этот пин
     * @param pinNum    номер пина
     * @return  параметр или nullptr
     */
    MWOSParam * FindByPin(MWOS_PIN_INT pinNum) {
        if (pinNum<0) return nullptr;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext && moduleNext->unitType==UnitType::MODULE) {
            MWOSParam *param=moduleNext->FindByPin(pinNum);
            if (param) return param;
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return nullptr;
    }

    uint32_t getLID() {
        if (CID>0) return CID;
        return getChipID();
    }

    /**
     * Получить глобальный CID контроллера. При необходимости, извлекает из дальнего хранилища (которое обычно не очищается).
     * @return  Глобальный CI контроллера. Если 0, то СID еще не выдан (не было соединений с сервером)
     */
    uint32_t getCID() {
        if (cid==0) {
#ifdef ESP32
            // загрузим CID из ESP NVS
            nvs_handle_t nvs_handle;
            if (nvs_open_from_partition(MWOS_CID_NVS,"mwos_net", NVS_READWRITE, &nvs_handle)==ESP_OK) {
                uint32_t savCid;
                if (nvs_get_u32(nvs_handle, "MWOS_CID", &savCid) == ESP_OK) {
                    cid=savCid;
                    MW_LOG_MODULE(this); MW_LOG(F("load NVS CID: ")); MW_LOG_LN(cid);
                }
            }
#else
            uint32_t savCid;
            if (MWEEPROM.readBuffer(MWOS_STORAGE_EEPROM_OFFSET,&savCid,4)==4) {
                cid=savCid;
                MW_LOG_MODULE(this); MW_LOG(F("load EEPROM CID: ")); MW_LOG_LN(cid);
            }
#endif
        }
        return cid;
    }

    bool saveCID(uint32_t newCID) {
        MW_LOG_MODULE(this); MW_LOG(F("Save CID: ")); MW_LOG_LN(cid);
#ifdef ESP32
        // сохраним CID в ESP NVS
        nvs_handle_t nvs_handle;
        if (nvs_open_from_partition(MWOS_CID_NVS,"mwos_net", NVS_READWRITE, &nvs_handle)==ESP_OK) {
            uint32_t savCid;
            if (nvs_get_u32(nvs_handle, "MWOS_CID", &savCid) != ESP_OK) { // еще не сохранено
                cid=newCID;
                nvs_set_u32(nvs_handle, "MWOS_CID", cid);
                MW_LOG_MODULE(this); MW_LOG(F("Save NVS CID: ")); MW_LOG_LN(cid);
                return true;
            }
        }
#else
        uint32_t savCid;
        if (MWEEPROM.readBuffer(MWOS_STORAGE_EEPROM_OFFSET,&savCid,4)!=4 || savCid==0 || savCid==UINT32_MAX || (CID>0 && savCid!=CID && newCID==CID)) {
            cid=newCID;
            MWEEPROM.writeBuffer(MWOS_STORAGE_EEPROM_OFFSET,&cid,4);
            MW_LOG_MODULE(this); MW_LOG(F("save EEPROM CID: ")); MW_LOG_LN(cid);
            return true;
        }
#endif
        return false;
    }
};
MWOS3 mwos;

#endif //MWOS3_MWOSBASE_H
