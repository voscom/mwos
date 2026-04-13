#ifndef MWOS3_MWOSSTORAGEFILE_H
#define MWOS3_MWOSSTORAGEFILE_H

#include "core/MWStorageRAM.h"
#include "fs/MWFS.h"

/***
 * Бинарное хранилище в файле, полностью кешируется в ОЗУ для быстрого доступа
 * Размер хранилища выбирается автоматически, в зависимости от модулей
 * загрузка из файла происходит при вызове onInit
 * сохранение в файл происходит при вызове commit
 */
class MWStorageFile : public MWStorageRAM {
public:
    MWOSFS * _fs;

    MWStorageFile(MWOSFS * forFS) : MWStorageRAM((char *) F("FILE")) {
        _fs=forFS;
    }

    virtual bool onInit(size_t bitSize, uint16_t crc16modules) {
        MWStorageRAM::onInit(bitSize,crc16modules); // создадим буфер
        size_t siz=0;
        if (_fs->fs->exists("/storage.ram")) { // загрузим из файла
            File file=_fs->fs->open("/storage.ram",MWFILE_READ);
            siz=file.read(_buff, size());
            file.close();
        }
        MW_LOG_TIME(); MW_LOG(F("File loaded: ")); MW_LOG_LN(siz);
        return MWStorageBytes::onInit(bitSize,crc16modules); // проверим контрольные суммы
    }

    /**
     * Комит изменений в хранилище
     * Вызывать из потомков вначале переопределения!
     * @param forced    Сохранить в любом случае (иначе, только если были изменения)
     * @return  Было сохранение
     */
    virtual bool commit(bool forced= false) {
        if (!_needCommit && !forced) return false;
        MWOSStorageRAM::commit(forced);
        File file=_fs->fs->open("/storage.ram",MWFILE_WRITE);
        file.seek(0);
        size_t siz=file.write(_buff, size());
        file.flush();
        file.close();
        MW_LOG_TIME(); MW_LOG(_num); MW_LOG(F(": '/storage.ram' - saved: ")); MW_LOG_LN(siz);
        return true;
    }


};


#endif //MWOS3_MWOSSTORAGEFILE_H
