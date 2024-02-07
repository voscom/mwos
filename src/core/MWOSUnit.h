#ifndef MWOS3_MWOSUNIT_H
#define MWOS3_MWOSUNIT_H
#include "MWOSUnitName.h"

#ifndef MWOS_PROJECT
#define MWOS_PROJECT "mwos3"
#endif
#ifndef GIT_REVISION_PROJ
#define GIT_REVISION_PROJ "-"
#endif
#ifndef GIT_REVISION_MWOS
#define GIT_REVISION_MWOS "-"
#endif
#ifndef BOARD
#define BOARD "-"
#endif
#ifndef GIT_URL_MWOS
#define GIT_URL_MWOS "-"
#endif
#ifndef GIT_URL_PROJ
#define GIT_URL_MWOS "-"
#endif
#ifndef MWOS_CRYPT_SECRET
#define MWOS_CRYPT_SECRET {0,0,0,0}
#endif


const char mwos_project[] PROGMEM = { MWOS_PROJECT }; // задается в конфигурации проекта define MWOS_PROJECT "mwos3"
const char git_hash_proj[] PROGMEM = { GIT_REVISION_PROJ }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini
const char git_hash_mwos[] PROGMEM = { GIT_REVISION_MWOS }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini
const char git_url_proj[] PROGMEM = { GIT_URL_PROJ }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini
const char git_url_mwos[] PROGMEM = { GIT_URL_MWOS }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini
const char mwos_board[] PROGMEM = { BOARD }; // BOARD из platformio.ini
const uint8_t mwos_secret_key[] PROGMEM = MWOS_CRYPT_SECRET; // BOARD из platformio.ini

/***
 *
 * Базовый класс всех объектов MWOS
 * представляет собой однонаправленный вперед список, в конце которого находится родитель (объект другого типа unitType)
 * родитель имеет тип MWOSParent и поле child с началом этого списка
 */

class MWOSUnit: public MWOSUnitName {
public:

#pragma pack(push,1)
    MWOS_PARAM_UINT id=0; // индентификатор объекта
    uint8_t  unitType: 4; // тип объекта UnitType (PARAM,MODULE,OS)
    uint8_t  storage0Init: 1; // хранилише 0 актуально
    uint8_t  storage1Init: 1; // хранилише 1 актуально
    uint8_t  storage2Init: 1; // хранилише 2 актуально
    uint8_t  storage3Init: 1; // хранилише 3 актуально
    uint8_t moduleType; // для модулей - тип модуля
    MWOSUnit * next=NULL; // следующий объект в списке (или родительский объект, для последнего в списке)
#pragma pack(pop)

    MWOSUnit(char * unit_name, uint16_t unit_id=0) : MWOSUnitName(unit_name) {
        unitType=PARAM;
        moduleType=MODULE_UNDEFINED;
        storage0Init= false;
        storage1Init= false;
        storage2Init= false;
        storage3Init= false;
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
