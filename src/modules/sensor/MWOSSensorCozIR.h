#ifndef MWOSSensorCozIR_H
#define MWOSSensorCozIR_H

#include "modules/sensor/MWOSSensorSerial.h"
// профиль CozIR - (надо проверить команды по даташиту)
const uint8_t MWOS_profileCozIR[] PROGMEM = { (uint8_t) BR_9600, 'Z',0,0,0,'e', 'K', ' ', '2',0 };

/**
 * Датчики CozIR - достаточно задать профиль
 * @tparam MWOSSensorAnalogType Обязательно <4>
 */
template<class MWOSSensorAnalogType = MWOSSensorAnalog<4>>
class MWOSSensorCozIR : public MWOSSensorSerial<4,MWOSSensorAnalogType> {
public:
    MWOSSensorLuminOx(Stream * serial) : MWOSSensorSerial<4,MWOSSensorAnalogType>(serial,&MWOS_profileCozIR) {
    }
};
#endif