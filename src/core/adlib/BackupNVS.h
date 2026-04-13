#ifndef OXYGEN_BACKUPNVS_H
#define OXYGEN_BACKUPNVS_H

#include <Arduino.h>
#include <nvs_flash.h>
#include "nvs.h"

/**
 * Для бекапа переменных в ESP NVS
 */
class BackupNVS {
public:

    nvs_handle_t nvs_handle;

    BackupNVS(const char *partition_label) {
        nvs_flash_init_partition(partition_label);
        nvs_open_from_partition(partition_label,"mwos", NVS_READWRITE, &nvs_handle);
    }

    void clear() {
        nvs_erase_all( nvs_handle);
    }

    /**
     * Получить сохраненное значение из NVS
      * @param key
      * @param defV Значение по умолчанию
      * @return
      */
    int64_t read(const char* key, int64_t defV) {
        int64_t res;
        if (nvs_get_i64(nvs_handle, key, &res) != ESP_OK) res=defV;
        return res;
    }

    String read(const char* key, String defV) {
        String res=defV;
        size_t siz=254;
        char buff[255];
        if (nvs_get_str(nvs_handle, key, (char*) &buff, &siz) == ESP_OK) res=String(buff,siz);
        return res;
    }

    /**
     *  Получить сохраненный массив из NVS
     * @param key
     * @param buff
     * @param size  Размер байт
     * @return  Считано байт
     */
    size_t read(const char* key, uint8_t * buff, size_t size) {
        size_t siz=size;
        if (nvs_get_blob(nvs_handle, key, buff,&siz) == ESP_OK) return siz;
        return 0;
    }

    /**
     * Сохранить значение в NVS
     * @param key
     * @param v
     * @return  Успешно
     */
    bool write(const char* key, int64_t v) {
        return nvs_set_i64(nvs_handle, key, v) == ESP_OK;
    }

    bool write(const char* key, String v) {
        size_t siz=v.length();
        return nvs_set_str(nvs_handle, v.c_str(), (const char *) &siz) == ESP_OK;
    }

    /**
     * Сохранить массив в NVS
     * @param key
     * @param buff
     * @param size  Размер байт
     * @return  Успешно
     */
    bool write(const char* key, uint8_t * buff, size_t size) {
        return nvs_set_blob(nvs_handle, key, buff,size) == ESP_OK;
    }

};


#endif //OXYGEN_BACKUPNVS_H
