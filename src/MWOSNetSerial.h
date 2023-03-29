#ifndef MWOS3_MWOSNETRSERIAL_H
#define MWOS3_MWOSNETRSERIAL_H
/**
 * Модуль связи по последовательному порту
 */
#define MWOS_SEND_BUFFER_USE 1 // использовать промежуточный буффер в модуле net
#include "core/net/MWOSNetDevice.h"

class MWOSNetSerial : public MWOSNetDevice {
public:

    MWOSNetSerial(Stream * stream=NULL) : MWOSNetDevice() {
        _stream=stream;
    }

    void SetSerial(Stream * stream) {
        _stream=stream;
    }

    virtual void startNet() { // аппаратно включить сеть
        setConnectedStep(8); // сразу перейдем к отправке хендшейка
    }



};


#endif //MWOS3_MWOSNETRSERIAL_H
