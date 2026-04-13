#ifndef MWOS3_MWOSPolledNet_H
#define MWOS3_MWOSPolledNet_H

#include <core/net/MWOSNet.h>

/**
 * Модуль polledNet - Polled Network (сеть с опросом) или сеть с управлением по принципу Master/Slave (Ведущий/Ведомый).
 * По очереди опрашивает подчиненные контроллеры в этой сети в режиме halfDuplex ({вкл передача}, {отправка данных}, {вкл прием}).
 */
template<uint8_t CIDS_LIST_SIZE=5, uint16_t MWNetReceiveBlockSize=1024, uint16_t MWNetSendBlockSize=1024>
class MWOSPolledNet : public MWOSNet<CIDS_LIST_SIZE,MWNetReceiveBlockSize,MWNetSendBlockSize> {
public:

    MWOSPolledNet() : MWOSNet<CIDS_LIST_SIZE,MWNetReceiveBlockSize,MWNetSendBlockSize>() {
        MWOSNet<CIDS_LIST_SIZE,MWNetReceiveBlockSize,MWNetSendBlockSize>::netType=1; // HalfDuplex
        MWOSNet<CIDS_LIST_SIZE,MWNetReceiveBlockSize,MWNetSendBlockSize>::pauseSend=1; // пока не отправляем
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_UPDATE) {
        }
        MWOSNet<CIDS_LIST_SIZE,MWNetReceiveBlockSize,MWNetSendBlockSize>::onEvent(modeEvent);
    }

    /**
     * Обработка событий от сети.
     * @param eventType Тип события
     * @return  Событие обработано успешно.
     */
    virtual bool onNetEvent(NetPacketEvent eventType) {
    }

    // Модуль занят приемом или передачей
    virtual bool IsBusy() {
        return false; // _timeout.isTimeout();
    }



};
#endif
