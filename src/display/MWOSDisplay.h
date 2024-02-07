#ifndef MWOS3_MWOSDISPLAY_H
#define MWOS3_MWOSDISPLAY_H
/***
 * Модуль экрана для показа данных на дисплее
 * Позволяет показывать виджеты на дисплей, совместимый с Adafruit-GFX-Library
 * Это не самостоятельный класс - дисплеи должны быть унаследованы от него
 * пример: MWOSDisplaySSD1306.h
 *
 * после вызова метода SetDisplayUpdate:
 * 1. очищает дисплей
 * 2. ждет перерисовки всех виджетов методом виджета print()
 * 3. на следующий кадр применяет изменения на дисплей
 *
 * Недостаток этого метода в том, что при каждом изменении любого виджета
 * - перерисовывает все виджеты, что может занять некоторое время
 * зато удобно и универсально!
 *
 * требует библиотеку: (нужно добавить в platform.ini)
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 *
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

// типы юнита
enum DisplayCmd : uint8_t {
    CMD_NO_ACTION = 0, // ничего не делать
    CMD_NEED_UPDATE = 1, // необходимо отрисовать все виджеты методом print
    CMD_UPDATE_NOW = 2 // необходимо применить вывод нового изображения для дисплея
};


class MWOSDisplay : public MWOSModule {
public:

#pragma pack(push,1)
    Adafruit_GFX * display;
    DisplayCmd cmd=CMD_NO_ACTION;
#pragma pack(pop)

    // занятые пины (параметр надо объявить в потомках)
    //MWOS_PARAM(0, pins_i2c, mwos_param_uint32, mwos_param_readonly+mwos_param_pin, mwos_param_storage_no, 3);
    //MWOS_PARAM(0, pins_spi, mwos_param_uint32, mwos_param_readonly+mwos_param_pin, mwos_param_storage_no, 7);

    // таймаут через который проверяется изменения значений (dSec [сек/10])
    MWOS_PARAM(1, size, mwos_param_uint16, mwos_param_option+mwos_param_readonly, mwos_param_storage_no, 2);

    MWOSDisplay() : MWOSModule((char *) F("display")) {
        AddParam(&p_size);
    }

    /**
     * Можно непосредственно задавать дисплей, не требующий комитов. Дисплей уже должен быть инициализован.
     * Но лучше использовать потомков (пример - MWOSDisplaySSD1306.h)
     * @param _display
     */
    MWOSDisplay(Adafruit_GFX * _display) : MWOSDisplay() {
        display=_display;
    }

    /**
     * Задать необходимость обновления дисплея
     * на следующий такт дисплей будет очищен, а все его виджеты - обновлены
     */
    void SetDisplayUpdate() {
        cmd=CMD_NEED_UPDATE;
    }

    bool IsNeedUpdated() {
        return cmd==CMD_UPDATE_NOW;
    }

    virtual void onInit() {
        clear();
        commit();
    }

    virtual void clear() { // очистить дисплей
        display->fillRect(0, 0, display->width()  - 1, display->height()  - 1, BLACK);
    }

    virtual void commit() { // применить изменения дисплея
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (cmd==CMD_UPDATE_NOW) {
            cmd=CMD_NO_ACTION;
            commit();
        } else
        if (cmd==CMD_NEED_UPDATE) {
            clear();
            cmd=CMD_UPDATE_NOW;
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param==&p_size) {
            if (arrayIndex==0) return display->width();
            if (arrayIndex==1) return display->height();
        }
        return MWOSModule::getValue(param,arrayIndex);
    }


};


#endif //MWOS3_MWOSDISPLAY_H
