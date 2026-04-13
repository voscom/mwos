#ifndef MWOS3_MWOSSTATUSCONNECT_H
#define MWOS3_MWOSSTATUSCONNECT_H
/***
 * Модуль статуса универсальный.
 * Поддерживает 1 или 2 светодиода (одноцветные и полноцветные).
 * В случае одноцветного - разные режимы мигания
 * В случае полноцветного - разные цвета
 *
 * Показывает статус для каждого светодиода. Оба светодиода (может быть задан только один)
 * имеют 8 состояний статуса:
 * 0 - тухнет - по умолчанию
 * 1 - [желтый] вспыхивает на 0.2 сек раз в 2 сек - идет подключение к сети
 * 2 - [красный] переключается каждые 0.2 сек - Подключено к сети WiFi или LAN
 * 3 - [оранжевый] вспыхивает на 0.2 сек раз в 0.8 сек - Идет подключение к серверу MWOS
 * 4 - [белый] переключается каждые 0.8 сек - Подключено к серверу MWOS
 * 5 - [фиолетовый] тухнет на 0.2 сек раз в 0.8 сек - Включена точка доступа и сервер WiFi
 * 6 - [синий] переключается каждые 2 сек - включен сервер Bluetooth или недавно посылали консольные команды cmd
 * 7 - [зеленый] тухнет на 0.2 сек раз в 2 сек (После получения пакета от радиосети, до отправки следующего запроса). Может тухнуть в промежутке между отправкой запроса и получения ответа.
 *
 * В [квадратных скобках] указан цвет для полноцветного индикатора (такой на платах esp32-s3-devkit)
 * По умолчанию светодиоды разделены так:
 * Светодиод 1: Статус WiFi и Bluetooth
 * Светодиод 2: Статус RadioNet
 * Если светодиод 2 не задан, то все статусы показывает на светодиоде 1.
 */

const uint8_t MWOSStatusConnectTimeoutDSecOn[] PROGMEM =  {  0,  2, 2, 2, 8, 8, 20, 20}; // на сколько [сек/10] включать индикатор для каждого режима
const uint8_t MWOSStatusConnectTimeoutDSecOff[] PROGMEM = { 50, 20, 2, 8, 8, 2, 20, 2 }; // на сколько [сек/10] выключать индикатор для каждого режима

#ifdef MWOS_LED_RGB
const uint32_t MWOSStatusConnectColor[] PROGMEM = { 0x000000, 0x404000, 0x400000, 0x402000, 0x404040, 0x004040, 0x000040, 0x0040040};
#endif

class MWOSStatusConnect : public MWOSModule {
public:

#pragma pack(push,1)
    uint16_t _status0:3; // текущий статус для индикатора 0
    uint16_t _status1:3; // текущий статус для индикатора 1
    uint16_t _lastConnectedStep:2; // тип подключения к сети и серверу (0-нет, 1-подключено к сети, 3-подключение к серверу, 2-подключено к серверу)
    uint16_t _showNet0:1;
    uint16_t _showNet1:1;
    uint16_t _showBLE0:1;
    uint16_t _showBLE1:1;
    uint16_t _showRadio0:1;
    uint16_t _showRadio1:1;
    uint16_t _nowIndicator0:1; // текущее состояние индикатора 0
    uint16_t _nowIndicator1:1; // текущее состояние индикатора 1
    MWOS_PIN_INT _pin[2];
#pragma pack(pop)

    // порты
    MWOS_PARAM(0, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, MWOS_STORAGE_FOR_PIN, 2);
    // показывать статус подключения к сети автоматически (для каждого индикатора)
    MWOS_PARAM(1, showNet, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 2);
    // показывать статус сервера Bluetooth автоматически (для каждого индикатора)
    MWOS_PARAM(2, showBLE, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 2);
    // показывать статус сети RadioNet автоматически (для каждого индикатора)
    MWOS_PARAM(3, showRadio, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 2);

    MWOSStatusConnect(MWOS_PIN_INT main_lcd_pin=-1, MWOS_PIN_INT second_lcd_pin=-1) : MWOSModule((char *) F("status")) {
        _pin[0]=main_lcd_pin;
        _pin[1]=second_lcd_pin;
        _showNet0=1;
        _showNet1=0;
        _showBLE0=1;
        _showBLE1=0;
        _showRadio0=(second_lcd_pin<0);
        _showRadio1=(second_lcd_pin>=0);
        AddParam(&p_pin);
        AddParam(&p_showNet);
        AddParam(&p_showBLE);
        AddParam(&p_showRadio);
#ifndef MWOS_LED_RGB
        if (_pin[0] >= 0) {
            mwos.mode(_pin[0], MW_PIN_MODE::MW_PIN_EVENT_OUTPUT);
            mwos.writeValueDigital(_pin[0],1);
        }
#endif

        if (_pin[1] >= 0) {
            mwos.mode(_pin[1], MW_PIN_MODE::MW_PIN_EVENT_OUTPUT);
            mwos.writeValueDigital(_pin[1],1);
        }
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _pin[0] = loadValue(_pin[0], &p_pin, 0);
            _pin[1] = loadValue(_pin[1], &p_pin, 1);
            _showNet0 = loadValue(_showNet0, &p_showNet, 0);
            _showNet1 = loadValue(_showNet1, &p_showNet, 1);
            _showBLE0 = loadValue(_showBLE0, &p_showBLE, 0);
            _showBLE1 = loadValue(_showBLE1, &p_showBLE, 1);
            _showRadio0 = loadValue(_showRadio0, &p_showRadio, 0);
            _showRadio1 = loadValue(_showRadio1, &p_showRadio, 1);
            if (_pin[0] >= 0) {
#ifndef MWOS_LED_RGB
                mwos.mode(_pin[0], MW_PIN_MODE::MW_PIN_EVENT_OUTPUT);
#endif

                if (modeEvent==EVENT_INIT) {
                    ShowLED0(1);
                    timeout0.start(10);
                }
            }
            if (_pin[1] >= 0) {
                mwos.mode(_pin[1], MW_PIN_MODE::MW_PIN_EVENT_OUTPUT);
                if (modeEvent==EVENT_INIT) {
                    ShowLED1(1);
                    timeout1.start(10);
                }
            }
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (_pin[0]<0) return;
            if (!timeout0.isTimeout() && !timeout1.isTimeout()) return; // оба таймаута не вышли
            MWOSNetModule * netModule=NULL;
            MWOSCmd * cmdModule=NULL;
            MWOSRadioNet * radioModule=NULL;
            int newStatus0=-1;
            if (_showNet0) {
                netModule=MWOSNetModule::getNetModule();
                if (netModule!=NULL) newStatus0=(int) netModule->GetStatus();
            }
            if (_showBLE0) {
                cmdModule=(MWOSCmd *) mwos.FindChildByName("cmd");
                if (cmdModule!=NULL) {
                    if (cmdModule->_status) newStatus0=6;
                    else if (newStatus0<0) newStatus0=0;
                }
            }
            if (_showRadio0) {
                radioModule=(MWOSRadioNet *) mwos.FindChildByName("radioNet");
                if (radioModule!=NULL) {
                    if (radioModule->_indicatorReceive) newStatus0=7;
                    else if (newStatus0<0) newStatus0=0;
                }
            }
            int newStatus1=-1;
            if (_pin[1]>=0) {
                if (_showNet1) {
                    if (netModule==NULL) netModule=MWOSNetModule::getNetModule();
                    if (netModule!=NULL) newStatus1=(int) netModule->GetStatus();
                }
                if (_showBLE1) {
                    if (cmdModule==NULL) cmdModule=(MWOSCmd *) mwos.FindChildByName("cmd");
                    if (cmdModule!=NULL) {
                        if (cmdModule->_status) newStatus1=6;
                        else if (newStatus1<0) newStatus1=0;
                    }
                }
                if (_showRadio1) {
                    if (radioModule==NULL) radioModule=(MWOSRadioNet *) mwos.FindChildByName("radioNet");
                    if (radioModule!=NULL) {
                        if (radioModule->_indicatorReceive) newStatus1=7;
                        else if (newStatus1<0) newStatus1=0;
                    }
                }
            }
            ShowStatus(newStatus0,newStatus1);
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (arrayIndex>1) return 0;
        switch (param->id) {
            case 0: _pin[arrayIndex]; break;
            case 1: if (arrayIndex==0) return _showNet0; else return _showNet1;
            case 2: if (arrayIndex==0) return _showBLE0; else return _showBLE1;
            case 3: if (arrayIndex==0) return _showRadio0; else return _showRadio1;
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /**
     * Показать на индикаторах новый статус (если статус -1 - то без изменений)
     * @param newStatus0    Для индикатора 0
     * @param newStatus1    Для индикатора 1
     */
    void ShowStatus(int newStatus0, int newStatus1) {
        if (newStatus0>=0 && newStatus0<8 && newStatus0!=_status0) {
            _status0=newStatus0;
            ShowLED0(1);
#if LOG_LED>0
            MW_LOG_MODULE(this); MW_LOG(F("ShowStatus0: ")); MW_LOG_LN(_status0);
#endif

        }
        if (newStatus1>=0 && newStatus1<8 && newStatus1!=_status1) {
            _status1=newStatus1;
            ShowLED1(1);
#if LOG_LED>0
            MW_LOG_MODULE(this); MW_LOG(F("ShowStatus1: ")); MW_LOG_LN(_status1);
#endif

        }
        // переключим статус по таймауту
        if (_pin[0]>=0 && timeout0.isTimeout()) ShowLED0(!_nowIndicator0);
        if (_pin[1]>=0 && timeout1.isTimeout()) ShowLED1(!_nowIndicator1);
    }

protected:
    MWTimeout timeout0;
    MWTimeout timeout1;

    int getTimeoutDSec(uint8_t _status, bool digitalValue) {
        if (_status>7) _status=0;
        uint8_t * arrTimes=(uint8_t *) &MWOSStatusConnectTimeoutDSecOff;
        if (digitalValue) // на сколько зажигать индикатор
            arrTimes=(uint8_t *) &MWOSStatusConnectTimeoutDSecOn;
        return pgm_read_byte_near(arrTimes + _status);
    }

#ifdef MWOS_LED_RGB
    uint32_t getColor(uint8_t _status) {
        if (_status>7) _status=0;
        uint8_t * arrColor= (uint8_t *) &MWOSStatusConnectColor;
        return pgm_read_dword_near(arrColor + _status*4);
    }
#endif

    void ShowLED0(bool digitalValue) {
        if (_pin[0]<0) return;
#ifdef MWOS_LED_RGB
        if (digitalValue) {
            uint32_t color=getColor(_status0);
            neopixelWrite(_pin[0],color >> 16 & 0xff,color >> 8 & 0xff,color & 0xff);
#if LOG_LED>1
            MW_LOG_MODULE(this); MW_LOG(F("RGB pin")); MW_LOG(_pin[0]);  MW_LOG('='); MW_LOG_LN(color,HEX);
#endif
        } else {
            neopixelWrite(_pin[0],0,0,0);
        }
#else
        mwos.writeValueDigital(_pin[0],digitalValue);
#endif
        _nowIndicator0=digitalValue;
        timeout0.start(getTimeoutDSec(_status0,digitalValue));
    }

    void ShowLED1(bool digitalValue) {
        if (_pin[1]<0) return;
        mwos.writeValueDigital(_pin[1],digitalValue);
        _nowIndicator1=digitalValue;
        timeout1.start(getTimeoutDSec(_status1,digitalValue));
    }

};


#endif //MWOS3_MWOSSTATUSCONNECT_H
