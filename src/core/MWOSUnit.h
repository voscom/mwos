#ifndef MWOS3_MWOSUNIT_H
#define MWOS3_MWOSUNIT_H
#include "MWOSUnitName.h"

#ifndef MWOS_PROJECT
#define MWOS_PROJECT "mwos3"
#endif
#ifndef MWOS_CRYPT_SECRET
#define MWOS_CRYPT_SECRET {0,0,0,0}
#endif

#ifndef MWOS_LIBS
#define MWOS_LIBS ""
#endif

#ifndef MWOS_BOARD
#define MWOS_BOARD ""
#endif

#ifndef MWOS_BOARD_FULL
#define MWOS_BOARD_FULL ""
#endif

#ifndef MWOS_BUILD_TIME
#define MWOS_BUILD_TIME 0
#endif

const char mwos_project[] PROGMEM = { MWOS_PROJECT }; // задается в конфигурации проекта define MWOS_PROJECT "mwos3"
const uint8_t mwos_secret_key[] PROGMEM = MWOS_CRYPT_SECRET; // BOARD из platformio.ini
const char mwos_board[] PROGMEM = { MWOS_BOARD }; // BOARD из platformio.ini
const char mwos_board_full[] PROGMEM = { MWOS_BOARD_FULL }; // ENV:BOARD из platformio.ini
const char mwos_libs[] PROGMEM = { MWOS_LIBS }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini

/***
 *
 * Базовый класс всех объектов MWOS
 * представляет собой однонаправленный вперед список, в конце которого находится родитель (объект другого типа unitType)
 * родитель имеет тип MWOSParent и поле child с началом этого списка
 */

class MWOSUnit: public MWOSUnitName {
public:

#pragma pack(push,1)
    MWOSUnit * next=NULL; // следующий объект в списке (или родительский объект, для последнего в списке)
    uint16_t id=0; // идентификатор объекта
    union { // 1 байт
        struct {
            uint8_t moduleType:6; // для модулей - тип модуля
            uint8_t unitType: 2; // тип объекта UnitType (PARAM,MODULE,OS)
        };
        struct {
            uint8_t paramType:6; // для параметров - тип параметра
            uint8_t : 2;
        };
    };
    union { // 1 байт
        uint8_t moduleReserve; // для модулей пока зарезервировано
        struct { // для параметров
            // 1-параметр изменен
            uint8_t changed: 1;
            // параметр отправлен и ожидает подтверждения
            uint8_t sended: 1;
            // необходимо отправить формат этого параметра или модуля
            uint8_t sendInf: 1;
            // признак, что параметр записан в хранилище
            uint8_t saved: 1;
            // этот параметр сохранять в журнал (битовая маска для всех параметров хранится в начале хранилища 0)
            uint8_t saveToLog:1;
            // Тип хранилища 0-7
            uint8_t storage:3;
        };
    };
#pragma pack(pop)

    MWOSUnit(char * unit_name, uint16_t unit_id=0) : MWOSUnitName(unit_name) {
        unitType=PARAM;
        moduleType=MODULE_UNDEFINED;
        id=unit_id;
    }

    /**
     * получить родителя этого объекта
     * @return родитель (или NULL, если нет)
     */
    MWOSUnit * getParent() { // получить родителя этого объекта
        MWOSUnit * unit=next;
        while (unit!=NULL && unit->unitType==unitType) unit=unit->next; // ищем конец списка
        return unit;
    }


};


#endif //MWOS3_MWOSUNIT_H
