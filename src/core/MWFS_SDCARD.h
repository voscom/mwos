#ifndef MWOS3_MWFS_SDCARD_H
#define MWOS3_MWFS_SDCARD_H
#include <SPI.h>
#include <SDFat.h>
#include "MWFS.h"

const char MWOS_FSNAME_EXFAT[] PROGMEM = {"exFAT"};
const char MWOS_FSNAME_FAT12[] PROGMEM = {"FAT12"};
const char MWOS_FSNAME_FAT16[] PROGMEM = {"FAT16"};
const char MWOS_FSNAME_FAT32[] PROGMEM = {"FAT32"};

/***
 * Файловая система FAT на SDCARD
 */
class MWFS_SDCARD: public MWFS {
public:
    File currentDir;

    MWFS_SDCARD(MWOS_PIN_INT csPin) : MWFS() {
        inited=sd.begin(csPin);
    }

    virtual void init() {
        if (!inited) return;
        uint8_t fatType=sd.vol()->fatType();
        switch (fatType) {
            case FAT_TYPE_EXFAT: name=(char *) &MWOS_FSNAME_EXFAT; break;
            case FAT_TYPE_FAT12: name=(char *) &MWOS_FSNAME_FAT12; break;
            case FAT_TYPE_FAT16: name=(char *) &MWOS_FSNAME_FAT16; break;
            case FAT_TYPE_FAT32: name=(char *) &MWOS_FSNAME_FAT32; break;
        }
    }

    virtual uint64_t size() {
        uint64_t clusters = sd.vol()->clusterCount();
        uint64_t clusterSize = sd.vol()->blocksPerCluster()*512;
        return clusters*clusterSize;
    }

    virtual uint64_t available() {
        uint64_t clusters = sd.vol()->freeClusterCount();
        uint64_t clusterSize = sd.vol()->blocksPerCluster()*512;
        return clusters*clusterSize;
    }

    virtual bool exists(const char* path) {
        return sd.exists(path);
    }

    virtual File open(const char* path, const uint8_t mode = FILE_READ) {
        return sd.open(path,mode);
    }

    virtual bool remove(const char* path) {
        return sd.remove(path);
    }

    virtual bool rename(const char* pathFrom, const char* pathTo) {
        return sd.rename(pathFrom,pathTo);
    }

    virtual bool mkdir(const char* path, bool createParentPath = true) {
        return sd.mkdir(path,createParentPath);
    }

    virtual bool rmdir(const char* path) {
        return sd.rmdir(path);
    }

    virtual bool chdir(const char *path) {
        currentDir=open(path);
        currentDir.rewind();
        nextFile=File();
        return sd.chdir(path);
    }

    virtual File * openNext(const uint8_t mode = FILE_READ) {
        nextFile=currentDir.openNextFile(mode);
        if (nextFile.isOpen()) return &nextFile;
        return NULL;
    }

    virtual bool format() {
        return sd.format();
    }

private:
    SdFat sd;
    File nextFile;
};

#endif //MWOS3_MWFS_SDCARD_H
