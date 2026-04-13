#ifndef MWOS3_MWStream_H
#define MWOS3_MWStream_H

#include <Stream.h>
#include "core/MWOSModuleBase.h"

/***
 * Расширенный класс передачи и приема данных
 * поддерживает прием и отправку блоков
 * поддерживает работу с несколькими модулями
 */
template<class MWClassForStream=Stream>
class MWStream : public MWClassForStream {
public:

    MWStream() : MWClassForStream()  {
    }

    /***
     * Прочитать блок данных (необходимо вызывать в основном цикле программы с большой периодичностью, пока не 0)
     * @param buff  Адрес буфера для чтения
     * @param maxSize  Максимальный размер принимаемого блока
     * @return  Количество считанных байт (0-нет)
     */
    virtual uint8_t ReadBlock(uint8_t * buff, uint8_t maxSize) {
        return 0;
    }

    /***
    * Отправить блок данных
    * @param buff Блок данных (до 8 байт)
    * @param size  Размер блока данных (1-8 байт)
    * @return  Сколько байт успешно отправлено
    */
    virtual uint8_t SendBlock(uint8_t * buff, uint8_t size) {
        return 0;
    }

    /**
     * Запись и чтение в поток доступно для этого модуля?
     * @return  Доступно
     */
    bool IsAvailableFor(MWOSModuleBase * module) {
        if (busyId<0) return true;
        if (module==NULL) return false;
        return module->id==busyId;
    }

    /**
     * Заблокировать только для заданного модуля
     * @param module    (NULL - разблокировать для всех)
     */
    void Lock(MWOSModuleBase * module=NULL) {
        if (module!=NULL) busyId=module->id;
        else busyId=-1;
    }

protected:
    int16_t busyId=-1;

};


#endif
