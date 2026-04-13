#ifndef MWOS3_MWFS_SDCARD_H
#define MWOS3_MWFS_SDCARD_H
#include "SPI.h"
#include "SdFat.h"
#define MWOS_SD_CARD
#define File SdFile
#include "MWFS.h"

const char MWOS_FSNAME_EXFAT[] PROGMEM = {"exFAT"};
const char MWOS_FSNAME_FAT12[] PROGMEM = {"FAT12"};
const char MWOS_FSNAME_FAT16[] PROGMEM = {"FAT16"};
const char MWOS_FSNAME_FAT32[] PROGMEM = {"FAT32"};

/***
 * Файловая система FAT на SDCARD
 *
 * Необходимая библиотека:
 * https://github.com/adafruit/SdFat.git
 *
 *
 */
class MWFS_SDCARD: public MWFS {
public:
    File currentDir;

    MWFS_SDCARD(MWOS_PIN_INT csPin=SS) : MWFS() {
        if (sd.begin(csPin, SPI_HALF_SPEED)) pin=csPin;
    }

    virtual bool init() {
        if (pin==-99) return false;
        uint8_t fatType=sd.vol()->fatType();
        switch (fatType) {
            case FAT_TYPE_EXFAT: name=(char *) &MWOS_FSNAME_EXFAT; break;
            case FAT_TYPE_FAT12: name=(char *) &MWOS_FSNAME_FAT12; break;
            case FAT_TYPE_FAT16: name=(char *) &MWOS_FSNAME_FAT16; break;
            case FAT_TYPE_FAT32: name=(char *) &MWOS_FSNAME_FAT32; break;
        }
        return true;
    }

    virtual uint64_t size() {
        uint64_t clusters = sd.vol()->clusterCount();
        uint64_t clusterSize = sd.vol()->sectorsPerCluster()*512;
        return clusters*clusterSize;
    }

    virtual uint64_t available() {
        uint64_t clusters = sd.vol()->freeClusterCount();
        uint64_t clusterSize = sd.vol()->sectorsPerCluster()*512;
        return clusters*clusterSize;
    }

    virtual bool exists(const char* path) {
        return sd.exists(path);
    }

    virtual File open(const char* path, const uint8_t mode = MWFILE_READ) {
        switch (mode) {
            case MWFILE_WRITE: return File(path, O_WRITE | O_CREAT | O_TRUNC);
            case MWFILE_WRITE_READ: return File(path, O_WRITE | O_CREAT | O_READ);
            case MWFILE_APPEND: return File(path, O_WRITE | O_CREAT);
        }
        return File(path,O_READ);
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

    virtual File * openNext(const uint8_t mode = MWFILE_READ) {
        nextFile.openNext(&currentDir, O_READ);
        if (nextFile.isOpen()) return &nextFile;
        return NULL;
    }

    virtual bool format() {
        return sd.format();
    }

    virtual String getFileName(File * file) {
        file->getName(fname,13);
        return String(fname);
    }

private:
    char fname[13];
    SdFat sd;
    File nextFile;
};

#endif //MWOS3_MWFS_SDCARD_H
