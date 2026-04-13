#ifndef MWOSNETFRAME_H
#define MWOSNETFRAME_H

#include "MWNetFrame.h"



class MWOSNetFrame: public MWNetFrame {

    /**
     * Обработать полученный сетевой фрейм
     * @return Фрейм успешно обработан и его можно удалять
     */
    bool Run() override {


        return false;
    }


};


#endif //MWOSNETFRAME_H
