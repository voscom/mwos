#ifndef MWOS3_MWOSNETCAN_H
#define MWOS3_MWOSNETCAN_H

#include "core/adlib/MWTimeout.h"
#include "core/hardware/MWBus_CAN.h"

#define timeoutSendCAN_mks 5 // таймаут (сек/1000) между отправками данных

/***
 * Упрощенный сетевой модуль для работы через сообщения 64-бит (например CANBUS)
 * Все пакеты связи сокращены до 8 байт, что-бы уложиться в одно сообщение 64 bit (структура сообщения в MWOS_NET_FRAME_64BIT)
 * Поддерживает до 255 модулей, до 255 параметров, до 255 индексов, и значения до int32 (int8,int16,int32,uint8,uint16)
 */
class MWOSNet64bit : public MWOSModule {
public:

    MWBus64bit * bus;

    // 64-бит данные последней полученной/отправленной команды
    MWOS_NET_FRAME_64BIT frameDat;

    /***
     * Упрощенный модуль работы через CAN
     * Поддерживает только обращение к параметрам int32
     */
    MWOSNet64bit() : MWOSModule((char *) F("net")) {
        moduleType=ModuleType::MODULE_NET;
    }

    /***
     * Упрощенный модуль работы через CAN
     * Поддерживает только обращение к параметрам int32
     * @param pinRx
     * @param pinTx
     * @param speedKBPS
     */
    MWOSNet64bit(MWBus64bit * bus64Bit) : MWOSNet64bit() {
        Init(bus64Bit);
    }

    void Init(MWBus64bit * bus64Bit)  {
        bus=bus64Bit;
    }

    /***
     * Поставить в очередль на отправку
     * @param moduleId
     * @param paramId
     * @param _value
     * @return
     */
    bool Send(int64_t value, uint16_t moduleId, uint16_t paramId,  uint16_t paramIndex) {
        frameDat.module_id=moduleId;
        frameDat.param_id=paramId;
        frameDat.value=value;
        frameDat.index=paramIndex;
        frameDat.cmd=0;
        bool res=bus->sendBlock((uint8_t *) &frameDat, 8);
        if (res) {
            MW_LOG_MODULE(this); MW_LOG(F("CAN sending: ")); MW_LOG(frameDat.module_id); MW_LOG('.'); MW_LOG(frameDat.param_id); MW_LOG('='); MW_LOG_LN((int32_t) frameDat.value);
        }
        timeoutSend.startMS(timeoutSendCAN_mks);
        return res;
    }

    /**
     * Получить новую команду по связи
     * @return  Команда получена в frameDat
     */
    bool Read() {
        int8_t size=bus->readBlock((uint8_t *) &frameDat);
        if (size>0) { // принимаем только по 8 байт!
            MWOSModuleBase * module= (MWOSModule *) mwos.getModule(frameDat.module_id);
            if (module!=NULL) {
                MWOSNetReciverFields reciverFields;
                reciverFields.param_id=frameDat.param_id;
                reciverFields.array_index=frameDat.index;
                reciverFields.reciveValue=frameDat.value;
                module->onReciveCmd((MWOSProtocolCommand) frameDat.cmd,&reciverFields);
                //module->setValueByParamID((int64_t) frameDat.value,(uint16_t) frameDat.param_id, frameDat.index);
            }
            return true;
        }
        return false;
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (Read()) {
            MW_LOG_MODULE(this); MW_LOG(F("CAN receive: ")); MW_LOG(frameDat.module_id); MW_LOG('.'); MW_LOG(frameDat.param_id); MW_LOG('='); MW_LOG_LN((int32_t) frameDat.value);
        }
        if (timeoutSend.isTimeout()) { // не чаще, чем через 10mks
            if (sendModule==NULL) {
                sendModule= (MWOSModule *) mwos.child;
                sendModule_param_id=0;
                sendModule_param_index=0;
            }
            if (sendModule_param_id<sendModule->paramsCount) {
                MWOSParam * param=sendModule->getParam(sendModule_param_id);
                if (param!=NULL && sendModule_param_index<param->arrayCount()) {
                    if (sendModule->IsParamChanged(param,sendModule_param_index)) {
                        if (Send(sendModule->getValueByParamID(sendModule_param_id),sendModule->id,sendModule_param_id,sendModule_param_index)) {
                            sendModule->SetParamChanged(param,sendModule_param_index, false); // если отправили параметр - сбросим признак изменения
                        }
                    }
                    sendModule_param_index++;
                } else {
                    sendModule_param_id++;
                    sendModule_param_index=0;
                }
            } else {
                sendModule=(MWOSModule *) sendModule->next;
                if (sendModule->unitType!=MODULE) sendModule=NULL;
                sendModule_param_id=0;
                sendModule_param_index=0;
            }

        }
    }

private:
    MWTimeout timeoutSend; // минимальный таймаут отправки значений
    MWOSModuleBase * sendModule=NULL; // текущий модуль для отправки параметров
    uint16_t sendModule_param_id=0;
    uint16_t sendModule_param_index=0;

};


#endif //MWOS3_MWOSNETCAN_H
