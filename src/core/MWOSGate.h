#ifndef MWOS3_GATE64BIT_H
#define MWOS3_GATE64BIT_H

#include "core/adlib/MWTimeout.h"

/***
 * Модуль гейта в подчиненную сеть

 Порядок авторизации подчиненного контроллера:

 * При обрыве связи, гейт рассылает всем подчиненным контроллерам признак разрыва связи (широковещанием или по очереди)

 * При восставновлении связи, гейт по очереди опрашивает все подчиненные контроллеры
 Контроллеры в ответ передают свои накопленные данные, но не более заданного ограничения размера.
 Данные от гейта передаются на сервер как обычный бинарный поток данных
 Кроме пакетов от подчиненного контроллера - главному контроллеру сети. Эти данные передаются напрямую контроллеру gate

 Если данные пришли от gate:net:номер_контроллера, то сервер считает их пришедшими от контроллера, закрепленным за этим параметром
 адрес параметра net прописывается в поле `controller`.`port` при первой авторизации контроллера
 При формировании пакета для подчиненного контроллера, пакет передается как бинарный поток на этот параметр net

 Подчиненные контроллеры не высылают данные сами, а только по запросу от гейта основного контроллера.
 Запрос от гейта (8 байт): 'mw' + (4 байта - адрес контроллера в подчиненной сети) + (2 байта - макимально допустимая длина ответа)
 Подчиненный контроллер должен ответить в течении 0.05 сек, иначе будет выдан запрос следующему подчиненному контроллеру



 */
template<MWOS_PARAM_INDEX_UINT controllersCount>
class MWOSGate : public MWOSModule {
public:

    MWStreamLog * sendBuffer; // кольцевой буффер отправляемых в подчиненную сеть данных

    // параметр для обмена данными с подчиненной сетью (каждый index соответствует подчиненному контроллеру)
    // сервер шлет на этот параметр данные для подчиненного контроллера и получает с него данные от подчиненного контроллера
    MWOS_PARAM(0, net, mwos_param_byte_array, mwos_param_control, mwos_param_storage_no, controllersCount);
    // адрес контроллера в подчиненной сети (что-то вроде IP, или адреса RS485). 0 = отключен.
    MWOS_PARAM(1, addr, mwos_param_int32, mwos_param_option, mwos_param_storage_eeprom, controllersCount);


    MWOSGate() : MWOSModule((char *) F("gate")) {
        moduleType=ModuleType::MODULE_GATE;
        AddParam(&p_net);
        AddParam(&p_addr);
        sendBuffer=new MWStreamLog(4096);
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->cmd == mwos_server_cmd_param_error_block) {
            //if (_oper != mwos_stream_oper_none) stop(mwos_stream_error_block); // блок прочитан с ошибкой
            return;
        }
        if (receiverDat->param_id == 0) {
            // сохраним в буфер для отправки (отправится из onEvent(Update))
            // {2b длина события (данные+индекс)} {2b индекс} {данные}
            if (sendBuffer->writeEventSize(receiverDat->size+2)) {
                sendBuffer->write((uint8_t *) &receiverDat->array_index,2);
                sendBuffer->write(receiverDat->buffer,receiverDat->size);
            } else {
                // error(1); // для этого пакета не хватает всего буфера
            }
            return;
        }
        MWOSModule::onReceiveValue(receiverDat);
    }

    /***
     * Вызывается при получении нового значения из гейта (вызывается в потомках)
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveFromGate(MWOSNetReceiverFields * receiverDat) {

    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {


    }


};


#endif //MWOS3_MWOSNETCAN_H
