#ifndef MWOS3_MWOSLoRaLR02_H
#define MWOS3_MWOSLoRaLR02_H

#include "../../core/MWOSModule.h"
#include "../../core/adlib/MWTimeout.h"
#include "../../core/MWOSRadioNet.h"

/**
 * Модуль LoRa DX-LR02-433T22D
 * без M0 и M1 (переключение командой "+++")
 * Беспроводная связь
 *
 */
class MWOSLoRaLR02 : public MWOSRadioNet {
public:

    enum InitStep : uint8_t {
        IS_STOP,
        IS_ERROR,
        IS_FIND_AT,
        IS_FIND_AT_WAIT,
        IS_MODE,
        IS_CANNEL,
        IS_MAC,
        IS_POWE,
        IS_LEVEL,
        IS_RESET,
        IS_DATA_MODE,
        IS_DATA_EVENT_WAIT,
    };

    enum LoRaAirDataRate: uint8_t {
        AR_244bps = 0,
        AR_447bps = 1,
        AR_813bps = 2,
        AR_1464bps = 3,
        AR_2604bps = 4,
        AR_4557bps = 5,
        AR_7812bps = 6,
        AR_13020bps = 7,
    };

    enum LoRaAirPower: uint8_t {
        APW_22dBm = 22,
        APW_17Bm = 17,
        APW_13Bm = 13,
        APW_10Bm = 10,
    };

#pragma pack(push,1)
    MWOS_PIN_INT _pin[3];
    uint8_t _power;
    uint8_t _channel=1;
    uint8_t _dataRate=0;
    // индекс чтения регистра
    uint8_t _loadRegIndex=0;
    InitStep _initStep;
    uint16_t _netAddress=0xff00;
#pragma pack(pop)
    String _answerStr;

    // порты модуля [0..2] Aux, M0, M1
    MWOS_PARAM(10, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, MWOS_STORAGE_FOR_PIN, 3);
    // отдельно настройка мощности
    MWOS_PARAM_FF(12, power, mwos_param_bits2, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "22=22dBm;17=17dBm;13=13dBm;10=10dBm");
    // Канал связи 410.125 + канал * 1M (в России разрешенный диапазон 433,075 - 434,775 МГц, значит только каналы 23 и 24)
    MWOS_PARAM(13, channel, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // Скорость передачи (0-2400 и т.д из LoRaAirDataRate)
    MWOS_PARAM_FF(14, dataRate, mwos_param_bits3, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "244 bps;447 bps;813 bps;1464 bps;2604 bps;4557 bps;7812 bps;13020 bps");

    /**
     * Создать модуль. Для каждого модуля необходим основной конструктор без аргументов!
     * конструкторы с аргументами - можно задавать дополнительно.
     * тут необходимо добавить все созданные параметры.
     * и можно переопределить имя и тип модуля.
     */
    MWOSLoRaLR02() : MWOSRadioNet() {
        // добавим параметры модуля
        AddParam(&p_pin);
        AddParam(&p_power);
        AddParam(&p_channel);
        AddParam(&p_dataRate);
        _maxPacketSize=50;
        _initStep=IS_STOP;
    }

    MWOSLoRaLR02(Stream * stream, MWOS_PIN_INT pinAux=-1, MWOS_PIN_INT pinM0=-1, MWOS_PIN_INT pinM1=-1) : MWOSLoRaLR02() {
        _stream=stream;
        _pin[0]=pinAux;
        _pin[1]=pinM0;
        _pin[2]=pinM1;
        setAddrDefault();
        setAirDefault();
    }


    /**
     * Задать параметры адресации и шифрования
     * @param e220address   Адрес модуля E220 (0xFFFF - для широковещания)
     */
    void setAddrDefault(uint16_t airAddress=0) {
        _netAddress=airAddress;
    }

    /**
     * Задать параметры связи по воздуху
     * @param airDataRate   Скорость
     * @param airChannel    [0..30] Канал связи (в России разрешенный диапазон 433,075 - 434,775 МГц, значит только каналы 0 и 1)
     * @param airPower      Мощность сигнала
     */
    void setAirDefault(LoRaAirDataRate airDataRate=AR_244bps, uint8_t airChannel=1, LoRaAirPower airPower=APW_22dBm) {
        _dataRate=(uint8_t) airDataRate;
        _power= (uint8_t) airPower;
        _channel=airChannel;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // только при инициализации системы
            for (uint8_t i = 0; i < 3; i++) {
                _pin[i]= loadValue(_pin[i], &p_pin, i);
            }
            mwos.mode(_pin[0],MW_PIN_EVENT_INPUT);
            _power= loadValue(_power, &p_power, 0);
            _channel= loadValue(_channel, &p_channel, 0);
            _dataRate= loadValue(_dataRate, &p_dataRate, 0);
            _maxCountSend= loadValue(_maxCountSend, &p_maxCountSend, 0);
            _maxCountReSend= loadValue(_maxCountReSend, &p_maxCountReSend, 0);
            _indicatorError=0;
            _status=RN_RECEIVE_OPTIONS;
            _initStep=IS_FIND_AT;
            _timeoutCmd.start(17);
        } else
        if (modeEvent==EVENT_UPDATE) {
            IsAux();
            if (_status==RN_RECEIVE_OPTIONS && _timeoutCmd.isTimeout()) {
                switch (_initStep) {
                    case IS_FIND_AT: {
                        sendText(F("AT"));
                    } break;
                    case IS_FIND_AT_WAIT: {
                        readText();
                        if (_answerStr.indexOf("OK")>=0) setInitStep(IS_MODE);
                        else if (_indicatorError==0) { // при первой ошибке - пытаемся переключиться в режим команд
                            _indicatorError=1;
                            sendText(F("+++"));
                            setInitStep(IS_FIND_AT); // и снова ищем ответ
                        } else setError(_initStep);
                    } break;
                    case IS_MODE: {
                        sendText(F("AT+MODE0"));
                    } break;
                    case IS_CANNEL: {
                        String ch=String(_channel,HEX);
                        if (ch.length()<2) ch="0"+ch;
                        sendText(String(F("AT+CHANNEL"))+ch);
                    } break;
                    case IS_MAC: {
                        String pr=String(_netAddress,HEX);
                        while (pr.length()<4) pr="0"+pr;
                        pr=pr.substring(0,2)+","+pr.substring(2);
                        sendText(String(F("AT+MAC"))+pr);
                    } break;
                    case IS_POWE: {
                        sendText(String(F("AT+POWE"))+String(_power));
                    } break;
                    case IS_LEVEL: {
                        sendText(String(F("AT+LEVEL"))+String(_dataRate));
                    } break;
                    case IS_RESET: {
                        sendText(F("AT+RESET"));
                    } break;
                    case IS_DATA_MODE: {
                        //sendText(F("+++"));
                        setInitStep(IS_DATA_EVENT_WAIT);
                    } break;
                    case IS_DATA_EVENT_WAIT: {
                        setInitStep(IS_STOP);
                        setStatus(RN_RECEIVE_DATA);
                    } break;
                }
            }
        }
        MWOSRadioNet::onEvent(modeEvent);
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
            case 10: return _pin[arrayIndex];
            case 12: return _power;
            case 13: return _channel;
            case 14: return _dataRate;
        }
        return MWOSRadioNet::getValue(param, arrayIndex);
    }

    /**
     * @return  модуль свободен и готов к приему новых команд или данных
     */
    virtual bool IsAux() {
        if (_pin[0]>=0) {
            bool aux=mwos.readValueDigital(_pin[0]);
#if LOG_RADIO>3
            if (_last_aux!=aux) {
                MW_LOG_MODULE(this); MW_LOG(F("AUX: ")); MW_LOG_LN(aux);
                _last_aux=aux;
            }
#endif
            return !aux;
        }
        return MWOSRadioNet::IsAux();
    }

protected:
    MWTimeout _timeoutCmd;
#if LOG_RADIO>3
    uint8_t _last_aux=0xff;
#endif

    // прочитать строку из буфера приема в _answerStr
    void readText() {
        while (_stream->available()>0) _answerStr+=(char) _stream->read();
#if LOG_RADIO>2
        MW_LOG_MODULE(this); MW_LOG(F("readText: ")); MW_LOG_LN(_answerStr);
#endif
    }

    void sendText(const String &str) {
        _stream->println(str);
#if LOG_RADIO>2
        MW_LOG_MODULE(this); MW_LOG(F("sendText: ")); MW_LOG_LN(str);
#endif

        setInitStep((InitStep) (((int) _initStep)+1));
    }

    void setInitStep(InitStep newStep) {
        _initStep=newStep;
        _timeoutCmd.start(3);
        readText();
        _answerStr="";
#if LOG_RADIO>2
        MW_LOG_MODULE(this); MW_LOG(F("step: ")); MW_LOG_LN((int16_t) newStep);
#endif
    }

    /**
     * @return Таймаут (сек/1000) отправки пакета
     */
    uint32_t getTimeoutSend() {
        float rate=12-_dataRate;
        float timeoutSend=5000.0f/12.0f*rate; // на скорости 2800bps передает пакет 240bytes максимум 2.8 сек
        return (uint32_t) round(timeoutSend);
    }


    void setError(int16_t errorCode) {
#if LOG_RADIO>0
         MW_LOG_LN(); MW_LOG_MODULE(this); MW_LOG(F("status: ")); MW_LOG(_status); MW_LOG(F(", error: ")); MW_LOG_LN(errorCode,HEX);
#endif

        _indicatorError=1;
        _initStep=IS_ERROR;
        _status=RN_NOT_INIT;
    }

};
#endif
