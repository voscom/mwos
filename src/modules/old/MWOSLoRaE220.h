#ifndef MWOS3_MWOSLoRaE220_H
#define MWOS3_MWOSLoRaE220_H

#include "../../core/MWOSModule.h"
#include "../../core/adlib/MWTimeout.h"
#include "../../core/MWOSRadioNet.h"

// значения регистров E220 по умолчанию
// const char MWOSLoRaE220_regDef[] PROGMEM = { 0, 0, 0x62, 0, 0x17, 0x03, 0, 0 , 0};

/**
 * Модуль LoRa E220
 * Беспроводная связь
 * Бывают с 8 или 9 регисторами (по умолчанию - 9)
   для модуля с 8 регисторами необходимо задать
  #define e220reg_count 8 // количество регистров
  перед инклудом

 * Значения регистров по умолчанию:
 * LoRaE220.12> reg=# 00 00 62 00 17 03 00 00
 * LoRaE220.12> reg=# 4D 0A 62 40 21 03 1A 7B
 *
 */
class MWOSLoRaE220 : public MWOSRadioNet {
public:

    enum LoRaAirDataRate: uint8_t {
        AR_2400bps = 2,
        AR_4800bps = 3,
        AR_9600bps = 4,
        AR_19200bps = 5,
        AR_38400bps = 6,
        AR_62500bps = 7,
    };

    enum LoRaAirPacketSize: uint8_t {
        AS_240bytes = 0,
        AS_128bytes = 1,
        AS_64bytes = 2,
        AS_32bytes = 3,
    };

    enum LoRaAirPower: uint8_t {
        APW_22dBm_30dBm = 0,
        APW_17dBm_27dBm = 1,
        APW_13dBm_24dBm = 2,
        APW_10dBm_21dBm = 3,
    };


#ifndef e220reg_count
// количество регистров (на E220 бывает 8 или 9)
#define e220reg_count 8
#endif
#if e220reg_count==9
#define netRegister 1 // длина регистора 2-NETID (0 или 1)
#else
#define netRegister 0 // длина регистора 2-NETID (0 или 1)
#endif
#define e220pin_count 3 // количество портов
#pragma pack(push,1)
    MWOS_PIN_INT _pin[e220pin_count];
    uint8_t _reg[e220reg_count+1];
    uint8_t _power;
    uint8_t _channel=24;
    uint8_t _dataRate=0;
    uint8_t _packetSize=0;
    // индекс чтения регистра
    uint8_t _loadRegIndex=0;
#pragma pack(pop)

    // порты модуля [0..2] Aux, M0, M1
    MWOS_PARAM(10, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, MWOS_STORAGE_FOR_PIN, e220pin_count);
    // регистры модуля [0..8]
    MWOS_PARAM(11, reg, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_NO, e220reg_count);
    // отдельно настройка мощности
    MWOS_PARAM_FF(12, power, mwos_param_bits2, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "22dBm_30dBm;17dBm_27dBm;13dBm_24dBm;10dBm_21dBm");
    // Канал связи 410.125 + канал * 1M (в России разрешенный диапазон 433,075 - 434,775 МГц, значит только каналы 23 и 24)
    MWOS_PARAM(13, channel, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // Скорость передачи (0-2400 и т.д из LoRaAirDataRate)
    MWOS_PARAM_FF(14, dataRate, mwos_param_bits3, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "2400 bps;4800 bps;9600 bps;19200 bps;38400 bps;62500 bps");
    // Размер пакета LoRaAirPacketSize
    MWOS_PARAM_FF(15, packetSize, mwos_param_bits3, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "240 bytes;128 bytes;64 bytes;32 bytes");

    /**
     * Создать модуль. Для каждого модуля необходим основной конструктор без аргументов!
     * конструкторы с аргументами - можно задавать дополнительно.
     * тут необходимо добавить все созданные параметры.
     * и можно переопределить имя и тип модуля.
     */
    MWOSLoRaE220() : MWOSRadioNet() {
        // добавим параметры модуля
        AddParam(&p_pin);
        AddParam(&p_reg);
        AddParam(&p_power);
        AddParam(&p_channel);
        AddParam(&p_dataRate);
        AddParam(&p_packetSize);
        _maxPacketSize=240;
    }

    MWOSLoRaE220(Stream * stream, MWOS_PIN_INT pinAux=-1, MWOS_PIN_INT pinM0=-1, MWOS_PIN_INT pinM1=-1) : MWOSLoRaE220() {
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
     * @param cryptKey      Ключ для шифрования беспроводного сигнала (только запись, на чтение - всегда 0)
     * @param netId         ID сети (можно задать -1, если у модуля нет регистора 2-NETID)
     */
    void setAddrDefault(uint16_t e220address=0, uint16_t cryptKey=0, uint8_t netId=0) {
        _reg[0]=(e220address >> 8) & 255; // ADDH（default 0)
        _reg[1]=e220address & 255; // ADDL（default 0)
        if (netRegister==1) {
            _reg[2]=netId; // NETID（default 0)
        }
        _reg[7+netRegister]=cryptKey & 255; // ADDL
        _reg[6+netRegister]=(cryptKey >> 8) & 255; // ADDH
    }

    /**
     * Задать параметры связи по воздуху
     * @param airDataRate   Скорость
     * @param airPacketSize Размер пакета
     * @param airChannel    [0..83] Канал связи 410.125 + канал * 1M (в России разрешенный диапазон 433,075 - 434,775 МГц, значит только каналы 23 и 24)
     * @param airPower      Мощность сигнала
     */
    void setAirDefault(LoRaAirDataRate airDataRate=AR_2400bps, LoRaAirPacketSize airPacketSize=AS_240bytes, uint8_t airChannel=23, LoRaAirPower airPower=APW_22dBm_30dBm) {
        _reg[2+netRegister]=0x60 + (uint8_t) airDataRate;
        _reg[3+netRegister]=((uint8_t) airPacketSize) << 6;
        _reg[3+netRegister] += (uint8_t) airPower;
        _power= (uint8_t) airPower;
        if (airChannel>83) airChannel=0;
        _reg[4+netRegister]=airChannel;
        _reg[5+netRegister]=3;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // только при инициализации системы
            for (uint8_t i = 0; i < e220pin_count; i++) {
                _pin[i]= loadValue(_pin[i], &p_pin, i);
            }
            mwos.mode(_pin[0],MW_PIN_EVENT_INPUT);
            mwos.mode(_pin[1],MW_PIN_EVENT_OUTPUT);
            mwos.mode(_pin[2],MW_PIN_EVENT_OUTPUT);
            _power= loadValue(_power, &p_power, 0);
            _channel= loadValue(_channel, &p_channel, 0);
            _dataRate= loadValue(_dataRate, &p_dataRate, 0);
            _packetSize= loadValue(_packetSize, &p_packetSize, 0);
            _maxCountSend= loadValue(_maxCountSend, &p_maxCountSend, 0);
            _maxCountReSend= loadValue(_maxCountReSend, &p_maxCountReSend, 0);
            setAirDefault((LoRaAirDataRate) (_dataRate+2), (LoRaAirPacketSize) _packetSize, _channel, (LoRaAirPower) _power);
            commandMode(true); // сохраним текущие регистры
            //if (_reg[0]!=0 && _reg[0]!=255) commandMode(true,true);
            //else commandMode(true,false);
        } else
        if (modeEvent==EVENT_UPDATE) {
            switch (_status) {
                case RN_SAVE_OPTIONS: saveRegisters(); break;
                case RN_LOAD_OPTIONS: loadRegisters(); break;
                case RN_RECEIVE_OPTIONS: readRegisters(); break;
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
            case 11: return _reg[arrayIndex];
            case 12: return _power;
            case 13: return _channel;
            case 14: return _dataRate;
            case 15: return _packetSize;
        }
        return MWOSRadioNet::getValue(param, arrayIndex);
    }

    /**
     * Загрузить все регистры E200 в массив _reg
     */
    void loadRegisters() {
        if (!_timeoutCmd.isTimeout() || !IsAux()) return; // дожидаемся выставления уровня на M0 и M1
        while (_stream->available()) _stream->read(); // прочитаем все байты из приемного буфера, что-бы его очистить
        _stream->write((uint8_t) 0xc1); // команда чтения регистров
        _stream->write((uint8_t) 0); // от регистра 0
        _stream->write((uint8_t) e220reg_count); // все регистры
#if LOG_RADIO>1
        MW_LOG_MODULE(this); MW_LOG(F("loadRegisters: ")); MW_LOG((uint8_t) 0xc1,HEX); MW_LOG(' '); MW_LOG((uint8_t) 0,HEX); MW_LOG(' '); MW_LOG_LN((uint8_t) e220reg_count,HEX);
#endif
        setStatus(RN_RECEIVE_OPTIONS);
        _loadRegIndex=0;
        _timeoutCmd.startMS(1000);
    }

    /**
     * Сохранить все регистры из массива _reg в E220
     */
    void saveRegisters() {
        if (!_timeoutCmd.isTimeout() || !IsAux()) return; // дожидаемся выставления уровня на M0 и M1
        while (_stream->available()) _stream->read(); // прочитаем все байты из приемного буфера, что-бы его очистить
        _stream->write((uint8_t) 0xc0); // команда записи регистров
        _stream->write((uint8_t) 0); // от регистра 0
        _stream->write((uint8_t) e220reg_count); // все регистры
#if LOG_RADIO>2
        MW_LOG_MODULE(this); MW_LOG(F("saveRegisters: ")); MW_LOG((uint8_t) 0xc0,HEX);  MW_LOG(' '); MW_LOG((uint8_t) 0,HEX); MW_LOG(' '); MW_LOG((uint8_t) e220reg_count,HEX);
#endif
        for (uint8_t i = 0; i < e220reg_count; i++) {
            _stream->write((uint8_t) _reg[i]);
#if LOG_RADIO>2
            MW_LOG(' '); MW_LOG((uint8_t) _reg[i], HEX);
#endif
        }
#if LOG_RADIO>2
        MW_LOG_LN();
#endif
        setStatus(RN_RECEIVE_OPTIONS);
        _loadRegIndex=0;
        _timeoutCmd.startMS(1000);
    }

    /**
     * @return  модуль свободен и готов к приему новых команд или данных
     */
    virtual bool IsAux() {
        if (_pin[0]>=0) return mwos.readValueDigital(_pin[0]);
        return MWOSRadioNet::IsAux();
    }

protected:
    MWTimeout _timeoutCmd;

    /**
     * @return Таймаут (сек/1000) отправки пакета
     */
    uint32_t getTimeoutSend() {
        float timeoutSend=2800.0f; // на скорости 2800bps передает пакет 240bytes максимум 2.8 сек
        // учтем размер пакета
        float maxPacketSize=13.0f;
        switch (_packetSize) {
            case AS_240bytes: maxPacketSize+=240; break;
            case AS_128bytes: maxPacketSize+=128; break;
            case AS_64bytes: maxPacketSize+=64; break;
            default: maxPacketSize+=32; break;
        }
        float packetCoef=253.0f/maxPacketSize;
        timeoutSend/=packetCoef;
        // учтем скорость передачи
        switch (_dataRate+2) {
            case AR_4800bps: timeoutSend/=2.0f; break;
            case AR_9600bps: timeoutSend/=4.0f; break;
            case AR_19200bps: timeoutSend/=8.0f; break;
            case AR_38400bps: timeoutSend/=16.0f; break;
            case AR_62500bps: timeoutSend/=24.0f; break;
        }
        return (uint32_t) round(timeoutSend);
    }

    /**
     * Перейти в командный режим и сохранить регистры
     * @param turnCommandMode   Командный режим вкл/выкл
     * @return  Командный режим доступен (если задан пинами)
     */
    bool commandMode(bool turnCommandMode) {
        if (_pin[1]<0 || _pin[2]<0) return false;
#if LOG_RADIO>3
        MW_LOG_MODULE(this); MW_LOG(F("commandMode=")); MW_LOG_LN(turnCommandMode);
#endif
        if (turnCommandMode) {
            mwos.writeValueDigital(_pin[1],HIGH);
            mwos.writeValueDigital(_pin[2],HIGH);
            setStatus(RN_SAVE_OPTIONS);
            _timeoutCmd.startMS(100);
        } else {
            mwos.writeValueDigital(_pin[1],LOW);
            mwos.writeValueDigital(_pin[2],LOW);
            setStatus(RN_WAIT_RECEIVE_DATA);
            while (_stream->available()) _stream->read(); // прочитаем все байты из приемного буфера, что-бы его очистить
        }
        return true;
    }

    void readRegisters() {
        if (_timeoutCmd.isTimeout()) {
            setError(-1);
            return;
        }
        if (_status==RN_RECEIVE_OPTIONS) {
            if (_loadRegIndex-3 >= e220reg_count) {
                onLoadReg();
            }
        }
        if (_stream->available()==0) return;
        uint8_t b=_stream->read();
#if LOG_RADIO>4
        MW_LOG('^'); MW_LOG(b,HEX);
#endif
        if (_loadRegIndex>=3) {
            int16_t k=_loadRegIndex-3;
            if (k >= e220reg_count) {
                onLoadReg();
            } else {
                _reg[k]=b;
                _loadRegIndex++;
            }
        } else
        if (_loadRegIndex==2) {
            if (b==e220reg_count) _loadRegIndex++;
            else setError(b);
        } else
        if (_loadRegIndex==1) {
            if (b==0) _loadRegIndex++;
            else setError(b);
        } else
        if (_loadRegIndex==0) {
            if (b==0xC1) _loadRegIndex++;
            else setError(b);
        }
    }

    void onLoadReg() {
#if LOG_RADIO>1
         MW_LOG_LN(); MW_LOG_MODULE(this); MW_LOG(F("reg=")); MW_LOG_BYTES((uint8_t *)&_reg,e220reg_count); MW_LOG_LN();
#endif
        _indicatorError=0;
        commandMode(false);
        _power=_reg[3+netRegister] & 3;
        SetParamChanged(&p_power);
        SetParamChanged(&p_reg);
    }

    void setError(int16_t errorCode) {
#if LOG_RADIO>0
         MW_LOG_LN(); MW_LOG_MODULE(this); MW_LOG(F("status: ")); MW_LOG(_status); MW_LOG(F(", error: ")); MW_LOG_LN(errorCode,HEX);
#endif
        _indicatorError=1;
        commandMode(true);
    }

};
#endif
