#ifndef SU_TRK_MWOSHalfDuplexPORT_H
#define SU_TRK_MWOSHalfDuplexPORT_H

#include <../../MWOSSerialPort.h>

const char MWOS_SERIAL_HALF_PORT_NAME[] PROGMEM = {"serialHalf"};

/**
 * Универсальный последовательный порт с дистанционной настройкой
 * для режима полудуплекс (с управлением направления)
 */
class MWOSHalfDuplexPort : public MWOSSerialPort {
public:

#pragma pack(push,1)
    uint8_t pinLineTX=GPIO_NUM_18;
    bool invertLineTX=false; // инверсия линни управления полудуплекса
    bool nowLineTX=false; // текущее состояние переключения на передачу (false - на прием)
#pragma pack(pop)
    Stream * _serial;

    // порт переключения полудуплекса
    MWOS_PARAM(5, pinLineTX, MWOS_PIN_INT_PTYPE, mwos_param_pin, MWOS_STORAGE_FOR_PIN, 1);
    // инверсия порта переключения полудуплекса (pinLineTX)
    MWOS_PARAM(6, invertLineTX, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // текущее состояние переключения полудуплекса на передачу (0 - на прием)
    MWOS_PARAM(7, nowLineTX, mwos_param_bits1, mwos_param_control, MWOS_STORAGE_RTC, 1);

    MWOSHalfDuplexPort(char * unit_name=(char *) &MWOS_SERIAL_HALF_PORT_NAME) : MWOSSerialPort(unit_name) {
        AddParam(&p_pinLineTX);
        AddParam(&p_invertLineTX);
        AddParam(&p_nowLineTX);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSHalfDuplexPort::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            invertLineTX = loadValue(invertLineTX, &p_invertLineTX, 0);
            pinLineTX = loadValue(pinLineTX, &p_pinLineTX, 0);
            nowLineTX = loadValue(nowLineTX, &p_nowLineTX, 0);
            if (pinLineTX >= 0) mwos.mode(pinLineTX, true, 0);
            setTransmit(nowLineTX);
        }
    }

    void setTransmit(bool v) {
        if (v == false) {
            delayMicroseconds(500);
            if (v!=nowLineTX) {
                nowLineTX=v;
                saveValue(nowLineTX,&p_nowLineTX,0);
                SetParamChanged(&p_nowLineTX);
            }
            //MW_LOG_MODULE(this); MW_LOG_LN(F("HalfDuplex: RX"));
        }
        if (pinLineTX >= 0) mwos.writeValueDigital(pinLineTX, v ^ invertLineTX);
        if (v == true) {
            delayMicroseconds(500);
            if (v!=nowLineTX) {
                nowLineTX=v;
                saveValue(nowLineTX,&p_nowLineTX,0);
                SetParamChanged(&p_nowLineTX);
            }
            //MW_LOG_MODULE(this); MW_LOG_LN(F("HalfDuplex: TX"));
        }
    }


};


#endif //SU_TRK_MWOSHalfDuplexPORT_H
