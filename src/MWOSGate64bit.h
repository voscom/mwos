#ifndef MWOS3_GATE64BIT_H
#define MWOS3_GATE64BIT_H

#include "core/adlib/MWTimeout.h"
#include "core/hardware/MWBUS_CAN.h"

/***
 * Упрощенный модуль гейта в подчиненную сеть для работы через CAN  (например CANBUS)
 * Все пакеты связи сокращены до 8 байт, что-бы уложиться в одно сообщение 64 bit (структура сообщения в MWOS_NET_FRAME_64BIT)
 * Поддерживает до 255 модулей, до 255 параметров, до 255 индексов, и значения до int32 (int8,int16,int32,uint8,uint16)
 * На параметрах задается subControllerId и модуль subControllerModuleId подчиненного контроллера
 * После этого, любое обращение к параметрам гейта, перенаправляется к параметрам заданного модуля подчиненного контроллера
 * Каждый модуль gate отвечает за один модуль подчиненного контроллера
 *
 */
class MWOSGate64bit : public MWOSModule {
public:

    uint32_t sub_controller_id;

    MWBus64bit * bus;

    // 64-бит данные последней полученной/отправленной команды
    MWOS_NET_FRAME_64BIT frameDat;

    // сетевой id подчиненного контроллера
    MWOS_PARAM(256, subControllerId, mwos_param_int32, mwos_param_option, mwos_param_storage_eeprom, 1);
    // количество модулей подчиненного контроллера (до 255)
    MWOS_PARAM(257, subControllerModuleId, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, 1);


    /***
     * Упрощенный гейт, для работы через сообщения 64 bit
     * Поддерживает только обращение к параметрам int32
     */
    MWOSGate64bit() : MWOSModule((char *) F("gate")) {

    }

    /***
     * Упрощенный модуль работы через CAN
     * Поддерживает только обращение к параметрам int32
     * @param pinRx
     * @param pinTx
     * @param speedKBPS
     */
    MWOSGate64bit(MWBus64bit * bus64Bit, uint32_t subControllerId) : MWOSGate64bit() {
        Init(bus64Bit,subControllerId);
    }

    void Init(MWBus64bit * bus64Bit, uint32_t subControllerId)  {
        sub_controller_id=subControllerId;
        bus=bus64Bit;
    }


    /**
     * Получить новую команду по связи
     * @return  Команда получена в frameDat
     */
    bool Read() {
        int8_t size=bus->readBlock((uint8_t *) &frameDat);
        if (size>0) { // принимаем только по 8 байт!
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
        }
    }

private:
    MWTimeout timeoutSend; // минимальный таймаут отправки значений

};


#endif //MWOS3_MWOSNETCAN_H
