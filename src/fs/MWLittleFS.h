#ifndef MWOS3_MWLittleFS_H
#define MWOS3_MWLittleFS_H
#define SPIFFS LITTLEFS
#include <core/MWOSDebug.h>

#include "LittleFS.h"
#include "MWFS.h"

const char MWOS_FSNAME_LITTLEFS[] PROGMEM = {"LittleFS"};

/***
 * Файловая система LittleFS на внутреннем програм-флеше ESP
 * ESP32
 * ESP8266
 * *
 * Разметка разделов:
 * partitions.csv
 *
 * board_build.filesystem = littlefs
 *
 * Использует встроенную библиотеку
 * lib_deps = LittleFS
 * esp32 - https://github.com/espressif/arduino-esp32/tree/master/libraries/LittleFS/
 *
 */
class MWLittleFS: public MWFS {
public:
#ifdef ESP8266
    Dir currentDir;
#else
    File currentDir;
#endif
    File currentFile;

    MWLittleFS() : MWFS() {
        name=(char *) &MWOS_FSNAME_LITTLEFS;
    }

    virtual bool init() {
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
        // Современные чипы: LittleFS с авто-форматированием
        if (LittleFS.begin(true)) {
            MW_LOG_LN(F("MWLittleFS inited!"));
            return true;
        } else {
            MW_LOG_LN(F("MWLittleFS init error!"));
            return false;
        }
#else
        LittleFS.begin();
#endif
        return true;
    }

    virtual uint64_t size() {
#ifdef ESP8266
        FSInfo info;
        LittleFS.info(info);
        return info.totalBytes;
#else
        return LittleFS.totalBytes();
#endif
    }

    virtual uint64_t available() {
#ifdef ESP8266
        FSInfo info;
        LittleFS.info(info);
        return info.totalBytes-info.usedBytes;
#else
        return LittleFS.totalBytes()-LittleFS.usedBytes();
#endif
    }

    virtual bool exists(const char* path) {
        return LittleFS.exists(path);
    }

    virtual File open(const char* path, const uint8_t mode = MWFILE_READ) {
        switch (mode) {
            case MWFILE_WRITE: return LittleFS.open(path,"w");
            case MWFILE_WRITE_READ: return LittleFS.open(path,"w+");
            case MWFILE_APPEND: return LittleFS.open(path,"a");
        }
        return LittleFS.open(path,"r");
    }

    virtual bool remove(const char* path) {
        return LittleFS.remove(path);
    }

    virtual bool rename(const char* pathFrom, const char* pathTo) {
        return LittleFS.rename(pathFrom,pathTo);
    }

    virtual bool mkdir(const char* path, bool createParentPath = true) {
        return LittleFS.mkdir(path);
    }

    virtual bool rmdir(const char* path) {
        return LittleFS.rmdir(path);
    }

    virtual bool chdir(const char *path) {
#ifdef ESP8266
        currentDir=LittleFS.openDir(path);
        currentFile=File();
        return currentDir.rewind();
#else
        if (path[0]!=0) currentDir=LittleFS.open(path);
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
        return LittleFS.format();
    }

    virtual String getFileName(File * file) {
        return String(file->name());
    }

    virtual void end() {
        LittleFS.end();
    }

private:
};

#endif //MWOS3_MWLittleFS_H
