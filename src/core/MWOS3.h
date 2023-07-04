//
// Created by mi on 08.07.2022.
//

#ifndef MWOS3_MWOSBASE_H
#define MWOS3_MWOSBASE_H

#include "adlib/MWArduinoLib.h"
#include "MWOSDebug.h"
#include "MWOSModuleBase.h"
#include "MWOSPinsMain.h"
#include "MWOSStorage.h"
#include "MWOSUnit.h"
#include "MWOSPins.h"
#include "MWOSParent.h"
#include "MWOSStorageRAM.h"
#include "MWOSStorageEEPROM.h"
#include "MWOSStorageStaticRAM.h"


#ifndef MWOS_DEBUG_BOUDRATE
#define MWOS_DEBUG_BOUDRATE 115200
#endif

#ifndef MWOS_PARAM_STORAGES_COUNT
#ifndef MWOSStorageStaticRAM_NOT
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
     * Можно задать информацию о контроллера
     * это могут быть данные в json-формате
     */
    char * info;

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
        // добавим хранилища (если не задано раньше)
#ifndef MWOS_NO_STORAGES
        if (storageCount==0) AddStorage(new MWOSStorageEEPROM()); // если не задано бинарное хранилище, то создадим хранилище в EEPROM
#ifndef MWOSStorageStaticRAM_NOT
        if (storageCount==1) AddStorage(new MWOSStorageStaticRAM()); // если задано только бинарное хранилище EEPROM, то создадим хранилище в StaticRAM (если платформа поддерживает)
#endif
#endif
        // создадим хранилища параметров
        for (int storageType = 0; storageType < storageCount; ++storageType) {
            uint32_t totalBitSize=0;
            MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
            while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
                totalBitSize+=moduleNext->bitsSize(storageType);
                moduleNext=(MWOSModuleBase *) moduleNext->next;
            }
            // сохраним
            if (!storages[storageType]->onInit(0,totalBitSize+16)) { // EEPROM новый
                MW_LOG(F("Storage save: ")); MW_LOG_LN(storageType);
                bitClear(storagesMask,storageType); // признак, что это хранилище ранее не было сохранено (настройки из базы данных серверера предпочтительнее)
                int32_t bitOffset=0; // необходимо сохранить все значения по умолчанию
                MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
                while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
                    totalBitSize+=moduleNext->bitsSize(storageType);
                    MWOSParam * param=(MWOSParam *) moduleNext->child;
                    while (param!=NULL && param->unitType==UnitType::PARAM) {
                        MW_LOG(F("MODULE: ")); MW_LOG(moduleNext->id); MW_LOG(F(", PARAM: ")); MW_LOG_LN(param->id);
                        if (param->storage==storageType) {
                            int16_t bitSize=param->bitsSize(false);
                            for (uint16_t index = 0; index < param->arrayCount(); ++index) {
                                storages[storageType]->saveValue(moduleNext->getValue(param, index), bitOffset, bitSize, false);   // сохранить значение параметра
                                bitOffset+=bitSize;
                            }
                        }
                        param=(MWOSParam *) moduleNext->next;
                    }
                    moduleNext=(MWOSModuleBase *) moduleNext->next;
                }
                storages[storageType]->commit();
            } else { // прочитаем все хранилища
                bitSet(storagesMask,storageType); // признак, что это хранилище ранее было успешно сохранено (его настройки предпочтительнее настроек сервера)
                MWOSModuleBase * moduleNext=(MWOSModuleBase *) child;
                while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
                    MWOSParam * param=(MWOSParam *) moduleNext->child;
                    while (param!=NULL && param->unitType==UnitType::PARAM) {
                        MW_LOG(F("MODULE: ")); MW_LOG(moduleNext->id); MW_LOG(F(", PARAM: ")); MW_LOG_LN(param->id);
                        for (uint16_t index = 0; index < param->arrayCount(); ++index) {
                            moduleNext->getValue(param,index); // вызовем запрос параметра
                        }
                        param=(MWOSParam *) moduleNext->next;
                    }
                    moduleNext=(MWOSModuleBase *) moduleNext->next;
                }

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
    MWOSPins * pin(uint16_t pinNum) {
        MWOSPins * res=&pins;
        while (res!=NULL) {
            if (res->isPin(pinNum)) return res;
            res=res->next;
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
                int16_t bitSize=param->bitsSize(false);
                bitOffset+=arrayIndex*bitSize; // добавим смещение массива внутри параметра
                return bitOffset;
            }
            bitOffset+=moduleNext->bitsSize(storageType);
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        return 0;
    }

    void saveValue(int64_t value, MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex=0) { // вызывается из модуля и из контроллера
        int8_t storageType=param->storage;
        if (storageType>=0 && storageType<storageCount) {
            uint32_t bitOffset=getBitOffset(module,param,arrayIndex); // найдем смещение в хранилище
            int16_t bitSize=param->bitsSize(false);
            storages[storageType]->saveValue(value,bitOffset,bitSize, true);
        }
    }

    int64_t loadValue(int64_t defValue, MWOSModuleBase * module, MWOSParam * param, int16_t arrayIndex=0) { // вызывается из модуля
        int8_t storageType=param->storage;
        if (storageType>=0 && storageType<storageCount) {
            uint32_t bitOffset=getBitOffset(module,param,arrayIndex); // найдем смещение в хранилище
            int16_t bitSize=param->bitsSize(false);
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
        while (moduleNext!=NULL && moduleNext->unitType==UnitType::MODULE) {
            moduleNext->onUpdate();
            moduleNext=(MWOSModuleBase *) moduleNext->next;
        }
        MW_DEBUG_UPDATE_FPS();
    }


};
MWOS3 mwos;

#endif //MWOS3_MWOSBASE_H
