#ifndef MWOS3_MWOSUNIT_H
#define MWOS3_MWOSUNIT_H

#ifndef MWOS_PROJECT
#define MWOS_PROJECT "test"
#endif
#ifndef GIT_REVISION_PROJ
#define GIT_REVISION_PROJ "test"
#endif
#ifndef GIT_REVISION_MWOS
#define GIT_REVISION_MWOS "test"
#endif
#ifndef BOARD
#define BOARD "test"
#endif

const char mwos_project[] PROGMEM = { MWOS_PROJECT }; // задается в конфигурации проекта define MWOS_PROJECT "mwos3"
const char git_hash_proj[] PROGMEM = { GIT_REVISION_PROJ }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini
const char git_hash_mwos[] PROGMEM = { GIT_REVISION_MWOS }; // необходимо добавить extra_scripts = pre:version_git.py в platformio.ini
const char mwos_board[] PROGMEM = { BOARD }; // необходимо добавить build_flags = -DBOARD=\"board-name\" в platformio.ini

/***
 *
 * Базовый класс всех объектов MWOS
 * представляет собой однонаправленный вперед список, в конце которого находится родитель (объект другого типа unitType)
 * родитель имеет тип MWOSParent и поле child с началом этого списка
 */

class MWOSUnit {
public:

#pragma pack(push,1)

    uint16_t id=0; // индентификатор объекта
    UnitType unitType=PARAM; // тип объекта
    uint8_t errorCode=0; // код ошибки последней операции (если была ошибка). Пока не используется. Оставлено для выравнивания до 32бит
    MWOSUnit * next=NULL; // следующий объект в списке (или родительский объект, для последнего в списке)
    char * name=NULL; // ссылка на имя или краткое описание в PROGMEM
#pragma pack(pop)

    MWOSUnit(char * unit_name, uint16_t unit_id=0) {
        id=unit_id;
        name=unit_name;
    }

    /***
     * Задать имя
     * @param unit_name
     */
    void setName(char * unit_name) {
        name=unit_name;
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

   /***
     * Возвращает размер имени в байтах
     * @return
     */
    size_t nameSize() {
        char * nameInRAM=name;
        if (nameInRAM==NULL) return 0;
        return strlen_P(nameInRAM);
    }

    /**
     * Напечатать строку из программной памяти
     * @param p Куда печатать
     * @param StrPROGMEM    Адрес строки в памяти
     * @param maxSize   Максимум символов, но не больше того что есть (0-всю строку)
     * @return  Сколько байт напечатано
     */
    size_t printStrPROGMEM(Print * p, char * StrPROGMEM, size_t maxSize=0) {
        if (StrPROGMEM==NULL) return 0;
        size_t size=strlen_P(StrPROGMEM);
        if (maxSize>0 && maxSize<size) size=maxSize;
        for (size_t i = 0; i < size; ++i) {
            char ch = pgm_read_byte_near(StrPROGMEM + i);
            p->print(ch);
        }
        return size;
    }

    size_t printName(Print * p) {
        char * nameInRAM=name;
        return printStrPROGMEM(p,nameInRAM);
    }

    /**
     * Сравнить имя с другим
     * @param nameInRAM2    Ссылка на другое имя в ПЗУ
     * @return Совпадают или нет
     */
    bool IsName(char * destName) {
        char * sourceName=name;
        if (sourceName==NULL) return false;
        size_t size=strlen_P(sourceName);
        size_t size2=strlen_P(destName);
        if (size!=size2) return false;
        for (size_t i = 0; i < size; ++i) {
            uint8_t ch1 = pgm_read_byte_near(sourceName + i);
            uint8_t ch2 = pgm_read_byte_near(destName + i);
            if (ch1!=ch2) return false;
        }
        return true;
    }

};


#endif //MWOS3_MWOSUNIT_H
