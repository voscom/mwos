#ifndef SU_TRK_MWOSSERIALPORT_H
#define SU_TRK_MWOSSERIALPORT_H

#include <SoftwareSerial.h>

const char MWOS_SERIAL_PORT_NAME[] PROGMEM = {"serial"};

// 5N1;6N1;7N1;8N1;5N2;6N2;7N2;8N2;5E1;6E1;7E1;8E1;5E2;6E2;7E2;8E2;5O1;6O1;7O1;8O1;5O2;6O2;7O2;8O2
const uint32_t SERIAL_CONFIGS[] PROGMEM = {
        SERIAL_5N1, // 0
        SERIAL_6N1, // 1
        SERIAL_7N1, // 2
        SERIAL_8N1, // 3
        SERIAL_5N2, // 4
        SERIAL_6N2, // 5
        SERIAL_7N2, // 6
        SERIAL_8N2, // 7
        SERIAL_5E1, // 8
        SERIAL_6E1, // 9
        SERIAL_7E1, // 10
        SERIAL_8E1, // 11
        SERIAL_5E2, // 12
        SERIAL_6E2, // 13
        SERIAL_7E2, // 14
        SERIAL_8E2, // 15
        SERIAL_5O1, // 16
        SERIAL_6O1, // 17
        SERIAL_7O1, // 18
        SERIAL_8O1, // 19
        SERIAL_5O2, // 20
        SERIAL_6O2, // 21
        SERIAL_7O2, // 22
        SERIAL_8O2 // 23
};

const uint32_t SERIAL_CONFIGS_SOFT[] PROGMEM = {
        SWSERIAL_5N1, // 0
        SWSERIAL_6N1, // 1
        SWSERIAL_7N1, // 2
        SWSERIAL_8N1, // 3
        SWSERIAL_5N2, // 4
        SWSERIAL_6N2, // 5
        SWSERIAL_7N2, // 6
        SWSERIAL_8N2, // 7
        SWSERIAL_5E1, // 8
        SWSERIAL_6E1, // 9
        SWSERIAL_7E1, // 10
        SWSERIAL_8E1, // 11
        SWSERIAL_5E2, // 12
        SWSERIAL_6E2, // 13
        SWSERIAL_7E2, // 14
        SWSERIAL_8E2, // 15
        SWSERIAL_5O1, // 16
        SWSERIAL_6O1, // 17
        SWSERIAL_7O1, // 18
        SWSERIAL_8O1, // 19
        SWSERIAL_5O2, // 20
        SWSERIAL_6O2, // 21
        SWSERIAL_7O2, // 22
        SWSERIAL_8O2 // 23
};

/**
 * Универсальный последовательный порт с дистанционной настройкой
 * для режимов дуплекс и симплекс
 */
class MWOSSerialPort : public MWOSModule {
public:

#pragma pack(push,1)
    uint8_t pinRX=GPIO_NUM_16;
    uint8_t pinTX=GPIO_NUM_17;
    uint32_t boudrate=115200;
    uint8_t protocol=2; // Serial2
    uint8_t config=3; // 8N1
    bool invertRX=0; // инверсия приема
#pragma pack(pop)
    Stream * _serial;

    // порты RX,TX
    MWOS_PARAM(0, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, MWOS_STORAGE_FOR_PIN, 2);
    // скорость порта
    MWOS_PARAM(1, boudrate, mwos_param_uint32, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // Serial0, Serial1, Serial2, Serial3, (далее можно использовать порты SPI,I2C,TWI и т.д.)
    MWOS_PARAM(2, protocol, mwos_param_bits3, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // конфигурация порта
    MWOS_PARAM(3, config, mwos_param_bits5, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // инверсия порта приема
    MWOS_PARAM(4, invertRX, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    MWOSSerialPort(char * unit_name=(char *) &MWOS_SERIAL_PORT_NAME) : MWOSModule(unit_name) {
        moduleType=MODULE_SERIAL;
        AddParam(&p_pin);
        AddParam(&p_protocol);
        AddParam(&p_boudrate);
        AddParam(&p_config);
        AddParam(&p_invertRX);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            pinRX = loadValue(pinRX, &p_pin, 0);
            pinTX = loadValue(pinTX, &p_pin, 1);
            invertRX = loadValue(invertRX, &p_invertRX, 0);
            boudrate = loadValue(boudrate, &p_boudrate, 0);
            protocol = loadValue(protocol, &p_protocol, 0);
            config = loadValue(config, &p_config, 0);
            switch (protocol) {
                case 0: {
                    Serial.begin(boudrate, pgm_read_dword_near(&SERIAL_CONFIGS[config]), pinRX, pinTX, invertRX);
                    _serial = &Serial;
                }
                    break;
                case 1: {
                    Serial1.begin(boudrate, pgm_read_dword_near(&SERIAL_CONFIGS[config]), pinRX, pinTX, invertRX);
                    _serial = &Serial1;
                }
                    break;
                case 2: {
                    Serial2.begin(boudrate, pgm_read_dword_near(&SERIAL_CONFIGS[config]), pinRX, pinTX, invertRX);
                    _serial = &Serial2;
                }
                    break;
                case 3: {
                    _serial = new EspSoftwareSerial::UART();
                    EspSoftwareSerial::Config cfgS = (EspSoftwareSerial::Config) pgm_read_dword_near(
                            &SERIAL_CONFIGS_SOFT[config]);
                    ((EspSoftwareSerial::UART *) _serial)->begin(boudrate, cfgS, pinRX, pinTX, invertRX);
                }
                    break;
                default: {
                    _serial = NULL;
                };
            }
        }
    }


};


#endif //SU_TRK_MWOSSERIALPORT_H
