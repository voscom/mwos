#ifndef MWOS3_MWOSNETMODULE_H
#define MWOS3_MWOSNETMODULE_H
/***
 * Работа с сервером без внешних устройств (по последовательному порту)
 * подключения к серверу, отключения от сервера, переподключения
 *
 *  --- Алгоритм взаимодействия с сервером:
    При подключении контроллера передает хендщейк, содержащий:
    id контроллера, хеш, версия прошивки, тип микропроцессора
    если id=0, то сервер сам раздаст контроллеру id
    если сервер обнаружит, что версия прошивки изменилась, он запросит формат контроллера
    при этом отправятся данные о всех модулях и параметрах.
    Получив блок формата контроллера сервер изменит его конфигурацию в базе
    id и формат всех модулей и параметро будут изменены по их именам
    недостающие будут добавлены, лишние - удалены
    после этого сервер готов принимать данные от параметров
 *
 *
 */

#include "MWOSNetReciver.h"


#ifndef MWOS_RESIVE_TIMEOUT_DSEC
#define MWOS_RESIVE_TIMEOUT_DSEC 600	// таймаут приема данных, если ничего не получили за это время - разрываем связь
#endif

#ifndef MWOS_SEND_TIMEOUT_DSEC
#define MWOS_SEND_TIMEOUT_DSEC 1	// таймаут между пакетами отправки данных
#endif

#ifndef MWOS_CONTROLLER_ID
#define MWOS_CONTROLLER_ID 0	// id контроллера по умолчанию
#endif

/**
 * Шаги подключения к серверу MWOS
 */
enum MWOSNetStep : int8_t {
    STEP_NET_CLOSE = 0,
    STEP_NET_CLOSING = 1,
    STEP_NET_START = 2,
    STEP_NET_STARTING = 3,
    STEP_NET_CONNECT = 4,
    STEP_NET_CONNECTING = 5,
    STEP_SERVER_CONNECT = 6,
    STEP_SERVER_CONNECTING = 7,
    STEP_SERVER_HANDSHAKE = 8,
    STEP_SERVER_WAIT_ID = 9,
    STEP_SERVER_CONNECTED = 15,
};

class MWOSNetModule : public MWOSModule, public MWOSNetReciver {
public:
#pragma pack(push,1)
    uint32_t cid=MWOS_CONTROLLER_ID; // код контроллера, присвоенный сервером при первой авторизации. MWOS_CONTROLLER_ID - это предпочитаемый код (если он еще не занят на серврере)
    MWOSNetStep connectedStep=STEP_NET_CLOSE;
#pragma pack(pop)

    // код контроллера
    MWOS_PARAM(0, controllerId, mwos_param_uint32, mwos_param_readonly, mwos_param_storage_eeprom, 1);

    MWOSNetModule(char * unit_name) : MWOSModule(unit_name) {
        AddParam(&p_controllerId);
        moduleType=ModuleType::MODULE_NET;
    }

    virtual void onInit() {
        MWOSModule::onInit();
        cid=loadValue(cid,&p_controllerId,0);
        MW_LOG_MODULE(this); MW_LOG(F("load id ")); MW_LOG_LN(cid);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex= 0) {
        if (param==&p_controllerId) return cid;
        return MWOSModule::getValue(param,arrayIndex);
    }

    virtual void onUpdate() {
        if (_stream != NULL) {
            if (_stream->available() > 0) {
                int16_t mx=1024; // читаем максимум за фрейм
                while (_stream->available() > 0 && --mx>0) { // получаем все данные
                    readNexByte(_stream->read());
                }
            } else
               sendUpdate();
            if (_cmdMode>0 && recive_block_timeout.isStarted() && recive_block_timeout.isTimeout()) reciveClear(20); // вышел таймаут приема
        }


        connectedStepRun();
    }

    virtual void connectedStepRun() {
    }

    virtual void onReciveTimeout() { // таймаут получения данных из порта
        MW_LOG_MODULE(this); MW_LOG_LN(F("lastReciveTimeout"));
        onDisconnect();
    }


    virtual void onConnect() {
        MW_LOG_MODULE(this); MW_LOG_LN(F("onConnect!"));
        setConnectedStep(STEP_SERVER_CONNECTED);
        if (!IsConnected) setStatusForGates(true);
        IsConnected=true;
    }

    virtual void onDisconnect() {
        MW_LOG_MODULE(this); MW_LOG_LN(F("onDisconnect!"));
#ifdef MWOS_SEND_BUFFER_USE
        sendOffset=0; // сбросим буффер отправки
        writeOffset=0;  // сбросим буффер отправки
#endif
        setConnectedStep(STEP_NET_CONNECTING); // перейдем на шаг подключения к серверу
        if (IsConnected) setStatusForGates(false);
        IsConnected=false;
    }

    void setConnectedStep(MWOSNetStep newStep) {
        connectedStep=newStep;
        MW_LOG_MODULE(this); MW_LOG(F("net step: ")); MW_LOG_LN(connectedStep);
    }

    virtual void setStatusForGates(bool connected) {


    }



};


#endif //MWOS3_MWOSNETMODULE_H

