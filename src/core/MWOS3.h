//
// Created by mi on 08.07.2022.
//

#ifndef MWOS3_MWOSBASE_H
#define MWOS3_MWOSBASE_H

#include "adlib/MWArduinoLib.h"
#include "MWOSDebug.h"
#include "MWOSModuleBase.h"
#include "pins/MWOSPinsMain.h"
#include "MWOSStorage.h"
#include "MWOSUnit.h"
#include "MWOSPins.h"
#include "MWOSParent.h"
#include "storages/MWOSStorageRAM.h"
#include "storages/MWOSStorageEEPROM.h"
#include "storages/MWOSStorageStaticRAM.h"
#include "core/MWOSConsts.h"


#ifndef MWOS_DEBUG_BOUDRATE
#define MWOS_DEBUG_BOUDRATE 115200
#endif

#ifndef MWOS_PARAM_STORAGES_COUNT
#ifndef MWOSStorageStaticRAM_NO
// максимальное количество хранилищ (на контроллерах с поддержкой StaticRAM)
#define MWOS_PARAM_STORAGES_COUNT 2
#else
// максимальное количество хранилищ (на контроллерах без поддержки StaticRAM)
#define MWOS_PARAM_STORAGES_COUNT 1
#endif
#endif

class MWOS3 : public MWOSParent {
public:

    MWOSStorage * storages[MWOS_PARAM_STORAGES_COUNT];
    uint8_t storageCount=0; // количество хранилищ
    uint8_t storagesMask; // битовые маски чтения хранилищ. Если хранилище 0 успешно прочитано (с совпадением контрольной суммы), то бит 0 = 1. И так для первых 8 хранилищ!
    uint16_t crc16modules; // контрольная сумма CRC16 по именам и количествам параметров всех модулей (для актуальности хранилищ)

    /**
     * общее количество модулей, включая модуль связи и модуль времени
     */
    uint16_t modulesCount=0;

    /***
     * Основной блок портов микроконтроллера (обычно порты 0-127)
     * Платы расширения добавлять так: mwos.pins.add(new MWOSPins());
     */
    MWOSPinsMain pins;

    /***
     * Модуль времени (если нет - NULL)
     * можно задать явно. Если не задали - ищет первый модуль типа MODULE_TIME
     */
    MWOSModuleBase * timeModule;

    /***
     * Модуль связи (если нет - NULL)
     * можно задать явно. Если не задали - ищет первый модуль типа MODULE_NET
     */
    MWOSModuleBase * netModule;

    MWOS3() : MWOSParent((char *) F("MWOS3")) {
        unitType=OS;
    }

    void start() {
        MW_DEBUG_BEGIN(MWOS_DEBUG_BOUDRATE);
        MW_LOG(F("MWOS3 start. Free mem: ")); MW_LOG_LN(getFreeMemory());
        randomSeed(analogRead(0));
        onInit();
    }

    void AddStorage(MWOSStorage * newStorage) {
        if (storageCount>=MWOS_PARAM_STORAGES_COUNT) return;
        storages[storageCount++]=newStorage;
    }

    /***
     * Вызывается автоматом перед первым Run (после создания всех модулей)
     */
    void onInit() {
        MW_LOG_LN();
        MW_LOG(F("Project git hash: ")); MW_LOG_PROGMEM((char *) &git_hash_proj); MW_LOG_LN();
        MW_LOG(F("MWOS git hash: ")); MW_LOG_PROGMEM((char *) &git_hash_mwos); MW_LOG_LN();
        randomSeed(analogRead(0));
        // добавим хранилища (если не задано раньше)
#ifndef MWOS_AUTO_STORAGES_NO
#ifndef MWOSStorageEEPROM_NO
        if (storageCount==0) AddStorage(new MWOSStorageEEPROM()); // если не задано бинарное хранилище, то создадим хранилище в EEPROM
#endif
#ifndef MWOSStorageStaticRAM_NO
        if (storageCount==1) AddStorage(new MWOSStorageStaticRAM()); // если задано только бинарное хранилище EEPROM, то создадим хранилище в StaticRAM (если платформа поддерживает)
#endif
#endif
        // контрольная сумма для актуальности хранилищ
        modulesCRC16();
        // создадим хранилища параметров
        for (uint8_t storageType = 0; storageType < storageCount; ++storageType) {
            uint32_t totalBitSize=0;
            MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
            while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
                totalBitSize+=moduleNext->bitsSize(storageType);
                moduleNext=(MWOSModuleBase *) moduleNext->next;
            }
            if (!storages[storageType]->onInit(totalBitSize+16,crc16modules)) { // пустое хранилище
                bitClear(storagesMask,storageType); // признак, что это хранилище ранее не было сохранено (настройки из базы данных серверера предпочтительнее)
                MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
                while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
                    MW_LOG(F("MODULE: ")); MW_LOG_PROGMEM(moduleNext->name); MW_LOG('='); MW_LOG_LN(moduleNext->paramsCount);
                    MWOSParam * param=(MWOSParam *) moduleNext->child;
                    while (param!=NULL && param->unitType==UnitType::PARAM) {
                        if (param->storage==storageType) {
                            MW_LOG(F("bits offset ")); MW_LOG_PROGMEM(moduleNext->name); MW_LOG(':'); MW_LOG_PROGMEM(param->name); MW_LOG_LN();
                            for (MWOS_PARAM_INDEX_UINT index = 0; index < param->arrayCount(); ++index) {
                                saveValue(moduleNext->getValue(param, index),moduleNext,param,index,false); // сохранить значение параметра
                            }
                        }
                        param=(MWOSParam *) param->next;
                    }
                    moduleNext=(MWOSModuleBase *) moduleNext->next;
                }
                storages[storageType]->commit(true);
                MW_LOG(F("Storage first saved: ")); MW_LOG_LN(storageType);
            } else { // прочитаем все хранилища
                MW_LOG(F("Storage restore ")); MW_LOG_LN(storageType);
                bitSet(storagesMask,storageType); // признак, что это хранилище ранее было успешно сохранено (его настройки предпочтительнее настроек сервера)
                /*
                MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
                while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
                    MWOSParam * param=(MWOSParam *) moduleNext->child;
                    while (param!=NULL && param->unitType==UnitType::PARAM) {
                        MW_LOG(F("MODULE: ")); MW_LOG(moduleNext->id); MW_LOG(F(", PARAM: ")); MW_LOG_LN(param->id);
                        for (MWOS_PARAM_INDEX_UINT index = 0; index < param->arrayCount(); ++index) {
                            moduleNext->getValue(param,index); // вызовем запрос параметра
                        }
                        param=(MWOSParam *) param->next;
                    }
                    moduleNext=(MWOSModuleBase *) moduleNext->next;
                }
                */
            }
        }
        // выделим место под маски признака изменения значений параметров
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
            moduleNext->changedMask.setSize(moduleNext->GetParamValuesCount());
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        MW_LOG(F("Storages initedSoft! Free mem: ")); MW_LOG_LN(getFreeMemory());
        // вызовем инициализацию модулей
        moduleNext=(MWOSModuleBase *) child;
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
            moduleNext->onInit();
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        if (timeModule==NULL) timeModule=(MWOSModuleBase *) FindChildByModuleType(MODULE_TIME);
        if (netModule==NULL) netModule=(MWOSModuleBase *) FindChildByModuleType(MODULE_NET);
        MW_LOG(F("MWOS3 initedSoft! Free mem: ")); MW_LOG_LN(getFreeMemory());
    }

    /***
     * Инициализировать хранилище, записав контрольную сумму и выставить признак, что хранилище актуально
     * @param storageType
     */
    void initStorage(uint8_t storageType) {
        storages[storageType]->initStorage(crc16modules);
        //storages[storageType]->commit();
        bitSet(storagesMask,storageType); // признак, что это хранилище ранее было успешно сохранено (его настройки предпочтительнее настроек сервера)
    }

    /***
     * Проверить, было ли инициализировано хранилище
     * @param storageType
     * @return
     */
    bool IsStorageInited(int8_t storageType) {
        return bitRead(storagesMask,storageType)>0;
    }

    /***
     * Расчитать контрольную сумму CRC16 по именам и количествам параметров всех модулей (для актуальности хранилищ)
     * @return  CRC16
     */
    void modulesCRC16() {
        MW_CRC16 crc16;
        crc16.start();
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
            char *nameInRAM = moduleNext->name; // контрольная сумма имени модуля
            for (uint8_t i = 0; i < strlen_P(nameInRAM); ++i) {
                char ch = pgm_read_byte_near(nameInRAM + i);
                crc16.add(ch);
            }
            crc16.add((moduleNext->paramsCount >> 8) & 255); // добавим CRC от количества парамтров
            crc16.add(moduleNext->paramsCount & 255);
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        crc16modules=crc16.crc;
        MW_LOG(F("modulesCRC16=")); MW_LOG_LN(crc16modules);
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
     * @return  Плата порта
     */
    MWOSPins * pin(MWOS_PIN_INT pinNum) {
        if (pinNum>=0) {
            MWOSPins * res=&pins;
            while (res!=NULL) {
                if (res->isPin(pinNum)) return res;
                res=res->next;
            }
        }
        return new MWOSPins(); // если не нашли порт - то пустой класс
    }

    /***
     * Рассчитать битовое смещение модуля и параметра для этого типа хранилища. От начала хранилища.
     * @param module
     * @param param
     * @param arrayIndex
     * @return
     */
   uint32_t getBitOffset(MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex=0) { // вызывается из модуля
        uint32_t bitOffset=0;
        int8_t storageType=param->storage;
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
            if (moduleNext==module) {
                bitOffset=moduleNext->getParamBitOffset(param,bitOffset); // добавим смещение параметра внутри модуля
                if (arrayIndex>0) {
                    int16_t bitSize=param->bitsSize(false);
                    bitOffset+=arrayIndex*bitSize; // добавим смещение массива внутри параметра
                }
                return bitOffset;
            }
            bitOffset+=moduleNext->bitsSize(storageType);
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return 0;
    }

    void saveValue(int64_t value, MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex=0, bool autoInited= true) { // вызывается из модуля и из контроллера
        int8_t storageType=param->storage;
        if (storageType>=0 && storageType<storageCount) {
            if (autoInited && !IsStorageInited(storageType)) initStorage(storageType);
            uint32_t bitOffset=getBitOffset(module,param,arrayIndex); // найдем смещение в хранилище
            int16_t bitSize=param->bitsSize(false);
            storages[storageType]->saveValue(value,bitOffset,bitSize);
        }
    }

    int64_t loadValue(int64_t defValue, MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex=0) { // вызывается из модуля
        int8_t storageType=param->storage;
        if (IsStorageInited(storageType) && storageType>=0 && storageType<storageCount) {
            uint32_t bitOffset=getBitOffset(module,param,arrayIndex); // найдем смещение в хранилище
            int16_t bitSize=param->bitsSize(false);
            MW_LOG_MODULE(module); MW_LOG("loadValue "); MW_LOG(param->id); MW_LOG(':'); MW_LOG_LN(arrayIndex);
            return storages[storageType]->loadValue(bitOffset,bitSize);
        }
        return defValue;
    }

    /***
     * Событие изменения значения для записи в журнал
     * @param value
     * @param param
     * @param arrayIndex
     */
    void toLog(int64_t value, MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex=0) {
        // TODO: тут надо сделать запись в журнал

    }

    void update() {
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
#ifdef MWOS_PROFILER
        MW_DEBUG_UPDATE_FPS();
        MWTimeout profilerTm;
#endif
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
#ifdef MWOS_PROFILER
            profilerTm.startMS(MWOS_PROFILER);
            uint32_t freeMem=getFreeMemory();
#endif
            moduleNext->onUpdate();
#ifdef MWOS_PROFILER
            if (profilerTm.isTimeout()) {
                MW_LOG_MODULE(moduleNext); MW_LOG("Profiler ALERT!!! time ms: "); MW_LOG_LN(profilerTm.msFromStart());
            }
            int32_t dMem=freeMem-getFreeMemory();
            if (dMem>0) {
                MW_LOG_MODULE(moduleNext); MW_LOG("Profiler ALERT!!! mem lost: "); MW_LOG_LN(dMem);
            }
#endif
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        for (uint8_t storageType = 0; storageType < storageCount; ++storageType) {
            storages[storageType]->commit(); // произвести комиты в хранилище, если были изменения
        }

    }


    /***
     * Ищет параметр, где используется этот пин
     * @param pinNum    номер пина
     * @return  параметр или NULL
     */
    MWOSParam * FindByPin(MWOS_PIN_INT pinNum) {
        MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
            MWOSParam *param=moduleNext->FindByPin(pinNum);
            if (param!=NULL) return param;
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return NULL;
    }
};
MWOS3 mwos;

#endif //MWOS3_MWOSBASE_H
