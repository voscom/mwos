#ifndef MWOS35_MWOSSENSORLUMINOX_H
#define MWOS35_MWOSSENSORLUMINOX_H
#include "modules/sensor/MWOSSensorSerial.h"
// профиль LuminOx - (надо проверить команды по даташиту)
const uint8_t MWOS_profileLuminOx[] PROGMEM = { (uint8_t) BR_9600,  '%','T','P','H','e', 'M', ' ', '1',0 };

/**
 * Датчики LuminOx - достаточно задать профиль
 * @tparam MWOSSensorAnalogType Обязательно <4>
 */
template<class MWOSSensorAnalogType = MWOSSensorAnalog<4>>
class MWOSSensorLuminOx : public MWOSSensorSerial<4,MWOSSensorAnalogType> {
public:
    MWOSSensorLuminOx(Stream * serial) : MWOSSensorSerial<4,MWOSSensorAnalogType>(serial,&MWOS_profileLuminOx) {
    }
};
#endif