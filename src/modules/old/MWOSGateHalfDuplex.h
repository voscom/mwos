#ifndef MWOS_MWOSGateHalfDuplex_H
#define MWOS_MWOSGateHalfDuplex_H

#include "../../MWOSHalfDuplexPort.h"

#ifndef MWOS_HALFDUPLEX_TIMEOUT_TRANSMIT_MS
#define MWOS_HALFDUPLEX_TIMEOUT_TRANSMIT_MS 80 // [сек/1000] время таймаута передачи полудуплекса
#endif

/**
 * Пересылает данные между дуплексом и полудуплексом
 * при приеме из дуплекса, переключает полудуплекс на отправку
 * переключение полудуплекса обратно на чтение - происходит по таймауту
 *
 */
class MWOSGateHalfDuplex : public MWOSHalfDuplexPort {
public:

#pragma pack(push,1)
    uint16_t timeoutMS=MWOS_HALFDUPLEX_TIMEOUT_TRANSMIT_MS;
#pragma pack(pop)
    MWOSSerialPort * _duplexSerial;
    MWTimeout _halfDuplexTimeout;

    // Id модуля с последовательным портом дуплекса
    MWOS_PARAM(7, serialPortModuleId, mwos_param_int16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // таймаут [сек/1000] передачи полудуплекса до авто-переключения на прием
    MWOS_PARAM(8, timeoutMS, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);

    MWOSGateHalfDuplex() : MWOSHalfDuplexPort((char *) F("gateHalfDuplex")) {
        AddParam(&p_serialPortModuleId);
        AddParam(&p_timeoutMS);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSHalfDuplexPort::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            timeoutMS = loadValue(timeoutMS, &p_timeoutMS, 0);
            int16_t serialPortModuleId = -1;
            if (_duplexSerial != NULL) serialPortModuleId = _duplexSerial->id;
            serialPortModuleId = loadValue(serialPortModuleId, &p_serialPortModuleId, 0);
            if (serialPortModuleId > 0) _duplexSerial = (MWOSSerialPort *) mwos.FindChildById(serialPortModuleId);
            if (_duplexSerial == NULL) {
                _duplexSerial = (MWOSSerialPort *) mwos.FindChildByModuleType(MODULE_SERIAL, this);
                if (_duplexSerial) {
                    saveValue(_duplexSerial->id, &p_serialPortModuleId, 0);
                    SetParamChanged(&p_serialPortModuleId, 0);
                }
            }
            if (serialPortModuleId >= 0 && _duplexSerial == NULL) {
                saveValue(-1, &p_serialPortModuleId, 0);
                SetParamChanged(&p_serialPortModuleId, 0);
            }
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (_duplexSerial != NULL && _duplexSerial->_serial!=NULL) while (_duplexSerial->_serial->available() > 0) onReceiveByteDuplexSerial(_duplexSerial->_serial->read());
            if (_serial!=NULL) while (_serial->available()>0) onReceiveByteHalfDuplex(_serial->read());
            if (nowLineTX && _halfDuplexTimeout.isTimeout()) setTransmit(false); // если линия полудуплекса на передаче больше таймаута - переключим ее на чтение
        }
    }

    void onReceiveByteDuplexSerial(uint8_t ch) { // получен байт из дуплекса
        if (!nowLineTX) setTransmit(true); // если линия полудуплекса на чтении - переключим ее на передачу
        if (_duplexSerial != NULL && _duplexSerial->_serial!=NULL) {
            _duplexSerial->_serial->write(ch);
            _halfDuplexTimeout.startMS(timeoutMS); // начнем таймаут передачи (от последнего переданного байта)
        }
    }

    void onReceiveByteHalfDuplex(uint8_t ch) { // получен байт из полудуплекса
        if (_duplexSerial != NULL && _duplexSerial->_serial!=NULL) _duplexSerial->_serial->write(ch); // переправим на дуплекс
    }


};


#endif //MWOS_MWOSGateHalfDuplex_H
