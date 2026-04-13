#ifndef MWOS3_MWOSUNIT_NAME_H
#define MWOS3_MWOSUNIT_NAME_H

/**
 * Строка для поиска начала имени в формате JSON
 */
const char progNameInJson[] PROGMEM = { "'name':'" };


/***
 * Базовый класс всех объектов с именами
 * содержит методы работы с именами
 */

#include <core/adlib/LibCRC.h>

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

    /**
     * Возвращает размер имени в байтах
     * @param sourceName    Начало имени в строке JSON (если поле name - JSON)
     * @return
     */
    size_t nameSize(char * sourceName=NULL) {
        if (name==NULL) return 0;
        if (sourceName==NULL || sourceName==name) return strlen_P(name);
        for (size_t i = 0; i < 100; i++) {
            char ch = pgm_read_byte_near(sourceName + i);
            if (ch==0 || ch==255 || ch=='\'') return i;
        }
        return strlen_P(name)-(name-sourceName);
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
     * Найти начало имени если строка в формате JSON, иначе - просто начало name
     * @return
     */
    char * getOnlyName() {
        char * nameInJson=(char *) &progNameInJson;
        char ch = pgm_read_byte_near(name);
        if (ch!='{') return name;
        size_t size1=strlen_P(name);
        size_t size2=strlen_P(nameInJson);
        int n=0;
        for (size_t i = 0; i < size1; ++i) {
            ch = pgm_read_byte_near(name + i);
            char ch2=pgm_read_byte_near(nameInJson + n);
            if (ch2==ch) n++;
            else n=0;
            if (n>=size2) return name + i + 1;
        }
        return name;
    }

    /**
     * Сравнить имя с другим
     * @param destName    Ссылка на другое имя
     * @param nameInProgMem    Ссылка на другое имя в ПЗУ? Иначе - в ОЗУ
     * @return Совпадают или нет
     */
    bool IsName(const char * destName, bool nameInProgMem=true) {
        if (name==NULL) return false;
        char * sourceName=getOnlyName();
        size_t size=nameSize(sourceName);
        size_t size2;
        if (nameInProgMem) size2=strlen_P(destName);
        else size2=strlen(destName);
        if (size!=size2) return false;
        for (size_t i = 0; i < size; ++i) {
            uint8_t ch1 = pgm_read_byte_near(sourceName + i);
            uint8_t ch2;
            if (nameInProgMem) ch2 = pgm_read_byte_near(destName + i);
            else ch2 = destName[i];
            if (ch1!=ch2 || ch2==0) return false;
        }
        return true;
    }

    /**
     * Добавить контрольную сумму имени
     * @param crc   Куда и как расчитывать CRC
     */
    void crcOfName(MW_CRC * crc) {
        char *nameInRAM = name; // контрольная сумма имени модуля
        size_t size=strlen_P(nameInRAM);
        for (size_t i = 0; i < size; ++i) {
            char ch = pgm_read_byte_near(nameInRAM + i);
            crc->add(ch);
        }
    }

};


#endif //MWOS3_MWOSUNIT_NAME_H
