#ifndef MWOS_FIRMWARE_H
#define MWOS_FIRMWARE_H
#include <Arduino.h>
#include <core/MWOSUnit.h>
/**
 * Возвращает краткую версию GIT прошивки
 */
String getVersionBuild() {
    // найдем версию гита
    int n=-1;
    char * progmem_str=(char *) mwos_libs;
    String res="";
    for (size_t i = 0; i < 100; i++) {
        char ch = pgm_read_byte_near(progmem_str + i);
        if (n>0) {
            res+=ch;
            if (n--==0) break;
        } else
            if (ch=='#') n=8;
    }
    return res;
    // дата прошивки и версия гита
    //return time0.toString("YY.MM.DD.",MWOS_BUILD_TIME)+res;
}

#endif //FIRMWARE_H
