#ifndef MWOS3_MWOS_OTA_H
#define MWOS3_MWOS_OTA_H

#include "../fs/MWOSStreamServer.h"

#define NO_OTA_NETWORK
#include <ArduinoOTA.h> // only for InternalStorage

/***
 * Модуль дистанционно обновления по воздуху (через WiFi, LAN, USB или другой модуль связи MWOS)
 *
 * Процесс обновления прошивки выглядит так:
 * Заливаем прошивку на сервер в папку upload или подпапки в upload, например в upload/firmware/firmware.bin
 * и отправляем в терминал контроллера команду заливки прошивки в контроллер:
> ota=$up;/firmware/firmware.bin
 * ждем, пока зальется и обновляем прошивку на залитую:
> ota:start:0=99
 *
 * Команды обновления (отправляются на ota:start:0):
 * 0 - остановить прием обновления (MWOSStreamReceive)
 * 1 - начать прием
 * -1 - сбросить ранее переданное обновление (только после успешно переданного обновления)
 * 99 - обновить прошивку на ранее переданное обновление и перезагрузиться (только после успешно переданного обновления)
 *
 * Supported micro-controllers:
classic ATmega AVR with at least 64 kB of flash (Arduino Mega, MegaCore MCUs, MightyCore 1284p and 644)
Arduino SAMD boards like Zero, M0, MKR and Nano 33 IoT
Arduino Uno R4 boards
nRF5 board supported by nRF5 core.
RP2040 boards with Pico core
STM32F boards with STM32 core
boards supported by ESP8266 and ESP32 Arduino boards package
any board with MCU with SD bootloader
any board with custom Storage and boot-load solution for that storage
Nano RP2040 Connect with mbed core with custom storage example and SFU library
 *
 * требует библиотеку:
 * https://github.com/JAndrassy/ArduinoOTA.git
 */
class MWOS_OTA : public MWOSStreamServer {
public:

    MWOS_OTA() : MWOSStreamServer((char *) F("ota")) {
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id == 0 && receiverDat->array_index == 0) {
            int8_t v=receiverDat->GetValueInt8();
            if (v==-1) {
                _complete= 0;
                SetParamChanged(&p_file,0, true);
                InternalStorage.clear();
                //InternalStorage.close();
                MW_LOG_MODULE(this); MW_LOG_LN(F("OTA clear!"));
            }
            if (v==99 && _complete) {
                wdt_disable();
#ifdef ESP32
                nvs_flash_erase(); nvs_flash_init(); // стирает NVS и EEPROM для ESP
#ifdef MWOS_NVS_PARTITION
                nvs_flash_erase_partition(MWOS_NVS_PARTITION); nvs_flash_init(); nvs_flash_init_partition("nvs1");
#endif
#else
                MWEEPROM.clear(MWOS_STORAGE_EEPROM_OFFSET); // стирает EEPROM
#endif
                InternalStorage.apply(); // обновить прошивку и перезагрузиться
            }
        }
        MWOSStreamServer::onReceiveValue(receiverDat);
    }

    virtual void onStreamEvent(MWOS_STREAM_CMD cmd) {
        switch (cmd) {
            case mwos_stream_cmd_start_receive: {
                if (_size > (uint32_t) InternalStorage.maxSize()) stop(mwos_stream_error_space);
                else {
                    if (!InternalStorage.open(_size)) {
                        MW_LOG_MODULE(this); MW_LOG(F("OTA error size: "));  MW_LOG_LN(_size);
                        stop(mwos_stream_error_size);
                        return;
                    }
                    MW_LOG_MODULE(this); MW_LOG(F("OTA: "));  MW_LOG_LN(_size);
                }
            } break;
            case mwos_stream_cmd_stop_receive: {
                InternalStorage.close(); // закончим запись в раздел OTA
                MW_LOG_MODULE(this); MW_LOG_LN(F("OTA.Close!"));
            } break;
            case mwos_stream_cmd_error: {
                //InternalStorage.clear();
            } break;
            default: {}
        }
    }

    virtual void onStreamReceiveBlock(uint8_t * buff, uint16_t size) {
        //MW_LOG_MODULE(this); MW_LOG(F("receive size: "));  MW_LOG_LN(size);
        for (uint16_t i = 0; i < size; i++) {
            InternalStorage.write(buff[i]); // побайтная запись в раздел OTA
        }
    }


};


#endif //MWOS3_MWOSKEY_H
