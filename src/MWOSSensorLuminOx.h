#ifndef MWOS3_MWOSSENSORSTREAM_H
#define MWOS3_MWOSSENSORSTREAM_H
/***
 * Аналоговый датчик LuminOx
 * показания датчика считываются с последовательного порта
 * O 0300.0 T +25.7 P 0959 % 025.00 e 0004
 *
 * формат значений параметра 9 (value)
0 - O - ppO2 in mbar (парциальное давления кислорода)
1 - T - temperature in °C
2 - P - current barometric pressure e in mbar (барометр)
3 - % - O2 in percent % (кислород в %)
4 - e - Sensor Status (0000 = Sensor Status Good, xxxx = Any other response)
 *
 */
#include "core/MWOSSensorAnalog.h"

/**
 * Количество контактов для подключенного устройства
 */
#define MWOSSensorStreamPinsCount 2

class MWOSSensorLuminOx : public MWOSSensorAnalog<5> {
public:
// типы юнита
    enum SerialParseCmd : uint8_t {
        SPC_WAIT_CMD = 0, // ожидание команды
        SPC_WAIT_SPACE = 1, // ожидание пробела после команды
        SPC_PARSE = 2 // парсинг значения
    };

    int32_t raw[5];
    int32_t avalue[5];
    MWOS_PIN_INT _pin[MWOSSensorStreamPinsCount];
    Stream * _stream;

    //************************ описание параметров ***********************/
    // порты, используемые для получения и отправки данных
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin+mwos_param_readonly, mwos_param_storage_no, MWOSSensorStreamPinsCount);

    MWOSSensorLuminOx(Stream * stream) : MWOSSensorAnalog<5>() {
        MWOSModuleBase::AddParam(&p_pin);
        _stream=stream;
        for (MWOS_PARAM_INDEX_UINT i = 0; i < MWOSSensorStreamPinsCount; ++i) {
            _pin[i]=-1;
        }
        for (uint8_t i = 0; i < 5; ++i) {
            raw[i]=0;
            avalue[i]=0;
        }
    }

     virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==1) return _pin[arrayIndex];
        return MWOSSensorAnalog<5>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    int8_t IndexOfCmd(char ch) {
        int8_t res=-1;
        switch (ch) {
            case 'O':
                res=0;
                break;
            case 'T':
                res=1;
                break;
            case 'P':
                res=2;
                break;
            case '%':
                res=3;
                break;
            case 'e':
                res=4;
                break;
        }
        raw[res]=0;
        return res;
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        while (_stream->available()>0) {
            char ch = _stream->read();
            switch (step) {
                case SPC_WAIT_CMD:
                    if (ch!=' ' && ch>13) {
                        indexCmd=IndexOfCmd(ch);
                        if (indexCmd<0) return;
                        //cmd=ch;
                        step=SPC_WAIT_SPACE;
                    }
                    break;
                case SPC_WAIT_SPACE:
                    if (indexCmd>=0 && ch==' ') step=SPC_PARSE;
                    else step=SPC_WAIT_CMD;
                    break;
                case SPC_PARSE:
                    if (ch==' ' || ch==13 || ch==10) { // закончим парсинг
                        avalue[indexCmd]=raw[indexCmd];
                        step=SPC_WAIT_CMD;
                        MW_LOG_MODULE(this); MW_LOG(indexCmd); MW_LOG('='); MW_LOG_LN(avalue[indexCmd]);
                    } else
                    if (ch>='0' && ch<='9') { // игнорируем все, кроме цифр
                        raw[indexCmd]*=10;
                        raw[indexCmd]+=(ch-'0');
                    }
                    break;
            }

        }

    }

    /**
     * Опросить аналоговый датчик для получения новых показаний
     */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        return avalue[arrayIndex];
    }

private:
    SerialParseCmd step=SPC_WAIT_CMD;
    //char cmd;
    int8_t indexCmd;

};


#endif //MWOS3_MWOSSENSORADC_H
