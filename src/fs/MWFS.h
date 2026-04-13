#ifndef MWOS3_MWFS_H
#define MWOS3_MWFS_H

#ifndef MWOS_SD_CARD
#include <FS.h>
#endif

#define MWFILE_READ 0
#define MWFILE_WRITE 1
#define MWFILE_APPEND 2
#define MWFILE_WRITE_READ 3

/**
 * Базовые функции файловой системы
 * файловая система должна быть наследником этого класса и определять все функции
 */
class MWFS : public MWOSUnitName {
public:
    MWOS_PIN_INT pin=-99;

    MWFS() : MWOSUnitName((char *) F("FS error!")) {
    }

    virtual bool init() {
        return false;
    }

    virtual uint32_t page() {
        return 2048;
    }

    virtual uint64_t size() {
        return 0;
    }

    virtual uint64_t available() {
        return 0;
    }

    uint64_t used() {
        return size()-available();
    }

    virtual bool exists(const char* path) {
        return false;
    }
    bool exists(const String& path) {
        return exists(path.c_str());
    }

    virtual File open(const char* path, const uint8_t mode = MWFILE_READ) {
        File file;
        return file;
    }
    File open(const String& path, const uint8_t mode = MWFILE_READ) {
        return open(path.c_str(),mode);
    }

    virtual bool remove(const char* path) {
        return false;
    }
    bool remove(const String& path) {
        return remove(path.c_str());
    }

    virtual bool rename(const char* pathFrom, const char* pathTo) {
        return false;
    }
    bool rename(const String& pathFrom, const String& pathTo) {
        return rename(pathFrom.c_str(),pathTo.c_str());
    }

    virtual bool mkdir(const char* path, bool createParentPath = true) {
        return false;
    }
    bool mkdir(const String& path, bool createParentPath = true) {
        return mkdir(path.c_str(),createParentPath);
    }

    virtual bool rmdir(const char* path) {
        return false;
    }
    bool rmdir(const String &path) {
        return rmdir(path.c_str());
    }

    /**
     * Перейти к папке
     * сбрасывает openNext к первому файлу в папке
     * @param path
     * @return
     */
    virtual bool chdir(const char *path) {
        return false;
    }
    bool chdir(const String &path) {
        return chdir(path.c_str());
    }

    /***
     * Открыть следующий файл в текущей папке (chdir сбрасывает к первому файлу в папке)
     * @param mode
     * @return  Ссылка на файл или NULL (если больше нет файлов)
     */
    virtual File * openNext(const uint8_t mode = MWFILE_READ) {
        return NULL;
    }

    virtual bool format() {
        return false;
    }

    virtual String getFileName(File * file) {
        return "";
    }

    virtual void end() {
    }



};

#endif //MWOS3_MWFS_H
