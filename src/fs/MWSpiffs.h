#ifndef MWOS3_MWSPIFFS_H
#define MWOS3_MWSPIFFS_H
#include "SPIFFS.h"
#include "MWFS.h"

const char MWOS_FSNAME_SPIFFS[] PROGMEM = {"SPIFFS"};

/***
 * Файловая система SPIFFS на внутреннем програм-флеше ESP
 * ESP32
 * ESP8266
 * *
 * Разметка разделов:
 * partitions.csv
 *
 * board_build.filesystem = spiffs
 *
 * Использует встроенную библиотеку
 * lib_deps = spiffs
 *
 */
class MWSpiffs: public MWFS {
public:
#ifdef ESP8266
    Dir currentDir;
#else
    File currentDir;
#endif
    File currentFile;

    MWSpiffs() : MWFS() {
        name=(char *) &MWOS_FSNAME_SPIFFS;
    }

    virtual bool init() {
        SPIFFS.begin(true);
        return true;
    }

    virtual uint64_t size() {
#ifdef ESP8266
        FSInfo info;
        SPIFFS.info(info);
        return info.totalBytes;
#else
        return SPIFFS.totalBytes();
#endif
    }

    virtual uint64_t available() {
#ifdef ESP8266
        FSInfo info;
        SPIFFS.info(info);
        return info.totalBytes-info.usedBytes;
#else
        return SPIFFS.totalBytes()-SPIFFS.usedBytes();
#endif
    }

    virtual bool exists(const char* path) {
        return SPIFFS.exists(path);
    }

    virtual File open(const char* path, const uint8_t mode = MWFILE_READ) {
        switch (mode) {
            case MWFILE_WRITE: return SPIFFS.open(path,"w");
            case MWFILE_WRITE_READ: return SPIFFS.open(path,"w+");
            case MWFILE_APPEND: return SPIFFS.open(path,"a");
        }
        return SPIFFS.open(path,"r");
    }

    virtual bool remove(const char* path) {
        return SPIFFS.remove(path);
    }

    virtual bool rename(const char* pathFrom, const char* pathTo) {
        return SPIFFS.rename(pathFrom,pathTo);
    }

    virtual bool mkdir(const char* path, bool createParentPath = true) {
        return SPIFFS.mkdir(path);
    }

    virtual bool rmdir(const char* path) {
        return SPIFFS.rmdir(path);
    }

    virtual bool chdir(const char *path) {
#ifdef ESP8266
        currentDir=SPIFFS.openDir(path);
        currentFile=File();
        return currentDir.rewind();
#else
        if (path[0]!=0) currentDir=SPIFFS.open(path,"r");
        if (currentDir && currentDir.isDirectory()) {
            currentDir.rewindDirectory();
            return true;
        }
#endif
        return false;
    }

    virtual File * openNext(const uint8_t mode = MWFILE_READ) {
#ifdef ESP8266
        if (!currentDir.next()) return NULL;
        switch (mode) {
            case MWFILE_WRITE: currentFile=currentDir.openFile("w"); break;
            case MWFILE_WRITE_READ: currentFile=currentDir.openFile("w+"); break;
            case MWFILE_APPEND: currentFile=currentDir.openFile("a"); break;
            default: currentFile=currentDir.openFile("r"); break;
        }
#else
        switch (mode) {
            case MWFILE_WRITE: currentFile=currentDir.openNextFile("w"); break;
            case MWFILE_WRITE_READ: currentFile=currentDir.openNextFile("w+"); break;
            case MWFILE_APPEND: currentFile=currentDir.openNextFile("a"); break;
            default: currentFile=currentDir.openNextFile("r"); break;
        }
#endif
        if (currentFile) return (File *) &currentFile;
        return NULL;
    }

    virtual bool format() {
        return SPIFFS.format();
    }

    virtual String getFileName(File * file) {
        return String(file->name());
    }

private:
};

#endif //MWOS3_MWSPIFFS_H
