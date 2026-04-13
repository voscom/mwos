#ifndef MWOSSENSORSPRINTFR_H
#define MWOSSENSORSPRINTFR_H

#include "modules/sensor/MWOSSensorSerial.h"
// профиль SprintIR - (надо проверить команды по даташиту)
const uint8_t MWOS_profileSprintIR[] PROGMEM = {  (uint8_t) BR_38400,  'Z','T','H',0,'e', 'K', ' ', '2',0 };

/**
 * Датчики SprintIR - достаточно задать профиль
 * @tparam MWOSSensorAnalogType Обязательно <4>
 */
template<class MWOSSensorAnalogType = MWOSSensorAnalog<4>>
class MWOSSensorLuminOx : public MWOSSensorSerial<4,MWOSSensorAnalogType> {
public:
    MWOSSensorLuminOx(Stream * serial) : MWOSSensorSerial<4,MWOSSensorAnalogType>(serial,&MWOS_profileSprintIR) {
    }
};
#endif