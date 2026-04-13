#ifndef MWOS3_MWOSMWOSSyncModule_H
#define MWOS3_MWOSMWOSSyncModule_H

#include "../../core/MWOSModule.h"
#include "../../core/adlib/MWTimeout.h"
#include "core/net/MWBusCrypt.h"

#pragma pack(push,1)
struct MWBusBlock {
    uint8_t syncId;
    int32_t v;
};
#pragma pack(pop)

#pragma pack(push,1)
struct MWSyncParamList {
    uint16_t moduleId;
    uint16_t paramId;
    uint16_t arrayIndex;
};
#pragma pack(pop)

/**
 * Модуль синхронизации значений параметров через шину данных.
 * Удобно использовать для синхронизации значений через радиоканал.
 *
 * По умолчанию использует шину данных MWBusCrypt
 * но можно задать любую совместимую
 *
 */
template<uint16_t sendParamsCount, uint16_t recParamsCount>
class MWOSSyncModule : public MWOSModule {
public:
    int8_t slave= 0; // это подчиненный контроллер
    int8_t lastRecBlock= 0; // последняя операция была успешное чтение блока данных
    uint32_t countSend=0;
    uint32_t countReceive=0;
    uint32_t lastCountReceive=0;
    uint32_t skipCountReceive=0;
    uint32_t timeReceive=0;
    uint32_t deltaTimeMax=0;
    uint64_t deltaTimeSumm=0;
    MWTimeout timeout;
    MWBus * _bus;
    // список параметров модулей для отправки значений
    MWSyncParamList sendSyncParamList[sendParamsCount];
    // список параметров модулей для получения значений
    MWSyncParamList recSyncParamList[recParamsCount];
    uint8_t buffSend[sendParamsCount* sizeof(MWBusBlock)];
    uint8_t buffRec[recParamsCount* sizeof(MWBusBlock)];

    // таймаут между отправками значений
    MWOS_PARAM(0, countSend, mwos_param_uint32, mwos_param_control + mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // количество принятых блоков
    MWOS_PARAM(1, countReceive, mwos_param_uint32, mwos_param_control + mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // количество пропущенных блоков
    MWOS_PARAM(2, countSkip, mwos_param_uint32, mwos_param_control + mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // время последнего принятого блока
    MWOS_PARAM(3, timeReceive, mwos_param_uint32, mwos_param_control + mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // максимальное время между полученными пакетами
    MWOS_PARAM(4, timeoutMax, mwos_param_uint32, mwos_param_control + mwos_param_readonly, MWOS_STORAGE_NO, 1);
    // среднее время между полученными пакетами
    MWOS_PARAM(5, timeoutMedium, mwos_param_uint32, mwos_param_control + mwos_param_readonly, MWOS_STORAGE_NO, 1);

    MWOSSyncModule() : MWOSModule((char *) F("syncModule")) {
        // добавим параметры модуля
        AddParam(&p_countSend);
        AddParam(&p_countReceive);
        AddParam(&p_countSkip);
        AddParam(&p_timeReceive);
        AddParam(&p_timeoutMax);
        AddParam(&p_timeoutMedium);
    }

    MWOSSyncModule(MWBus * bus) : MWOSSyncModule() {
        _bus=bus;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // только при инициализации системы
             timeout.startMS(random(2000,2100));
        } else if (modeEvent==EVENT_UPDATE) {
            if (_bus->IsAvailableFor(this) && _bus->dataAvailable()) {
                uint16_t recSize=_bus->readBlock((uint8_t *) &buffRec, sizeof(buffRec));
                if (recSize>0) { // получен новый блок
                    onReceiveValuesFromBus(recSize);
                }
                timeout.startMS(100); // если ничего не получали 0.1 сек - начнем отправку
            } else
            if (timeout.isTimeout()) {
                if (slave<1 || lastRecBlock>slave) nowSendValuesToBus();
                else lastRecBlock++;
                timeout.startMS(random(1000,1100)); // сделаем таймаут немного случайным, что-бы оправки контроллеров случайно не попали на одно время
            }
        }
        MWOSModule::onEvent(modeEvent);
    }

    /**
     * Необходимо переопределить в потомках!
     * Тут можно вызывать несколько SendValueToBus
     */
    void nowSendValuesToBus() {
        for (uint8_t i = 0; i < sendParamsCount; ++i) if (sendSyncParamList[i].moduleId>0 && sendSyncParamList[i].moduleId<255) {
            MWOSModuleBase * module=mwos.getModule(sendSyncParamList[i].moduleId);
            MWBusBlock block;
            block.syncId=i;
            if (module!=NULL) block.v=module->getValueByParamId(sendSyncParamList[i].paramId, sendSyncParamList[i].arrayIndex);
            else block.v=0;
            memcpy(&buffSend[i*sizeof(MWBusBlock)],&block,sizeof(MWBusBlock));
        }
        if (lastCountReceive==countReceive) {
            skipCountReceive++;
            SetParamChanged(&p_countSkip);
        } else lastCountReceive=countReceive;
        lastRecBlock= 0;
#ifdef LOG_SYNC_MODULE
        MW_LOG_MODULE(this); MW_LOG(countSend); MW_LOG(F("; sendBus:")); MW_LOG_LN(sizeof(buffSend));
#endif
        countSend++;
        SetParamChanged(&p_countSend);
        _bus->sendBlock((uint8_t *) &buffSend, sizeof(buffSend)); // отправим блок
    }

    void onReceiveValuesFromBus(uint16_t recSize) {
#ifdef LOG_SYNC_MODULE
        MW_LOG_MODULE(this); MW_LOG(countReceive); MW_LOG(F("; recBus:")); MW_LOG_LN(recSize);
#endif
        lastRecBlock= slave+1;
        countReceive++;
        uint32_t timeReceiveNow=MWOSTime::getTimeModule()->getValueByParamId(4,0);
        int32_t deltaTime=timeReceiveNow-timeReceive;
        if (deltaTime>0 && deltaTime<3600*24*30) {
            if (deltaTime>deltaTimeMax) {
                deltaTimeMax=deltaTime;
                SetParamChanged(&p_timeoutMax);
            }
            deltaTimeSumm+=deltaTime;
            SetParamChanged(&p_timeoutMedium);
        }
        timeReceive=timeReceiveNow;
        SetParamChanged(&p_countReceive);
        SetParamChanged(&p_timeReceive);
        for (int i = 0; i < recParamsCount; i++) {
            onReceiveValueFromBus((MWBusBlock *) &buffRec[i*sizeof(MWBusBlock)]);
        }
    }

    /**
     * Получен новый блок данных
     */
    void onReceiveValueFromBus(MWBusBlock * receiveBlock) {
        if (receiveBlock->syncId>=recParamsCount) return;
#ifdef LOG_SYNC_MODULE
        MW_LOG_MODULE(this); MW_LOG(countReceive); MW_LOG(F(";recBus ")); MW_LOG(receiveBlock->syncId); MW_LOG('='); MW_LOG_LN(receiveBlock->v);
#endif
        MWOSModuleBase * module=mwos.getModule(recSyncParamList[receiveBlock->syncId].moduleId);
        if (module!=NULL) {
            module->setValueByParamId(receiveBlock->v,recSyncParamList[receiveBlock->syncId].paramId,recSyncParamList[receiveBlock->syncId].arrayIndex); // установим полученное значение
        }
    }

    /**
      * Вызывается при запросе значения параметра
      * Читает значение из хранилища (кроме байтовых массивов - из надо читать вручную)
      * @param paramNum  Номер параметра
      * @param arrayIndex Номер индекса в массиве значений параметра (если это массив)
      * @return  Значение
      */
    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 0: return countSend;
            case 1: return countReceive;
            case 2: return skipCountReceive;
            case 3: return timeReceive;
            case 4: return deltaTimeMax;
            case 5: { if (countReceive>0) return deltaTimeSumm/countReceive; else return 0; }
        }
        return MWOSModule::getValue(param, arrayIndex);
    }

    /**
     * Добавить параметр к списку получаемых
     * @param syncIndex Порядковый номер этого параметра в списке на получение
     * @param moduleId
     * @param paramId
     * @param arrayIndex
     * @return  syncIndex или -1, если ошибка
     */
    int16_t AddRecSyncParam(int16_t syncIndex, uint16_t moduleId, uint16_t paramId, uint16_t arrayIndex) {
        if (syncIndex>=recParamsCount || syncIndex<0) return -1;
        recSyncParamList[syncIndex].moduleId=moduleId;
        recSyncParamList[syncIndex].paramId=paramId;
        recSyncParamList[syncIndex].arrayIndex=arrayIndex;
        return syncIndex;
    }

    /**
     * Добавить несколько значений массива одного параметра к списку получаемых
     * @param syncIndexFirst    Первый порядковый номер в списке на получение
     * @param moduleId
     * @param paramId
     * @param arrayIndexFrom    С какого индекса значения
     * @param arrayIndexTo      До какого индекса значения
     * @return  Последний добавленный порядковый номер в списке на получение
     */
    int16_t AddRecSyncParamArray(int16_t syncIndexFirst, uint16_t moduleId, uint16_t paramId, uint16_t arrayIndexFrom, uint16_t arrayIndexTo) {
        for (uint16_t i = arrayIndexFrom; i <= arrayIndexTo; i++) {
            AddRecSyncParam(syncIndexFirst+i-arrayIndexFrom,moduleId,paramId,i);
        }
        return syncIndexFirst+arrayIndexTo-arrayIndexFrom;
    }

    /**
     * Добавить параметр к списку отправляемых
     * @param syncIndex Порядковый номер этого параметра в списке на отправку
     * @param moduleId
     * @param paramId
     * @param arrayIndex
     * @return  syncIndex или -1, если ошибка
     */
    int16_t AddSendSyncParam(int16_t syncIndex, uint16_t moduleId, uint16_t paramId, uint16_t arrayIndex) {
        if (syncIndex>=sendParamsCount || syncIndex<0) return -1;
        sendSyncParamList[syncIndex].moduleId=moduleId;
        sendSyncParamList[syncIndex].paramId=paramId;
        sendSyncParamList[syncIndex].arrayIndex=arrayIndex;
        return syncIndex;
    }

    /**
     * Добавить несколько значений массива одного параметра к списку отправляемых
     * @param syncIndexFirst    Первый порядковый номер в списке на отправку
     * @param moduleId
     * @param paramId
     * @param arrayIndexFrom    С какого индекса значения
     * @param arrayIndexTo      До какого индекса значения
     * @return  Последний добавленный порядковый номер в списке на отправку
     */
    int16_t AddSendSyncParamArray(int16_t syncIndexFirst, uint16_t moduleId, uint16_t paramId, uint16_t arrayIndexFrom, uint16_t arrayIndexTo) {
        for (uint16_t i = arrayIndexFrom; i <= arrayIndexTo; i++) {
            AddSendSyncParam(syncIndexFirst+i-arrayIndexFrom,moduleId,paramId,i);
        }
        return syncIndexFirst+arrayIndexTo-arrayIndexFrom;
    }

    /**
     * Отправить значение параметра модуля на шину данных
     * @param module
     * @param syncId    порядковый номер параметра модуля в списке синхронизации
     * @return  Количество отправленных байт
     */
    uint8_t SendValueToBus(int32_t v, uint16_t syncId) {
        MWBusBlock block;
        block.syncId=syncId;
        block.v=v;
#ifdef LOG_SYNC_MODULE
        MW_LOG_MODULE(this); MW_LOG(countSend); MW_LOG(F(";sendBus ")); MW_LOG(syncId); MW_LOG('='); MW_LOG_LN(block.v);
#endif
        countSend++;
        return _bus->sendBlock((uint8_t *) &block, sizeof(block)); // отправим блок
    }

};


#endif
