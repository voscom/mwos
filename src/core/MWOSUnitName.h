#ifndef MWOS3_MWOSUNIT_NAME_H
#define MWOS3_MWOSUNIT_NAME_H

/***
 * Базовый класс всех объектов с именами
 * содержит методы работы с именами
 */

class MWOSUnitName {
public:
    char * name=NULL; // ссылка на имя или краткое описание в PROGMEM

    MWOSUnitName(char * unit_name=NULL) {
        name=unit_name;
    }

    /***
     * Задать имя
     * @param unit_name
     */
    void setName(char * unit_name) {
        name=unit_name;
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
        for (size_t i = 0; i < size; i++) {
            char ch = pgm_read_byte_near(StrPROGMEM + i);
            p->write(ch);
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


#endif //MWOS3_MWOSUNIT_NAME_H
