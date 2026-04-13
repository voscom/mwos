#include "Arduino.h"

/**
 * Позволяет выбирать дополнительные шрифты
 *
 * Доп.шрифты тут:
 * https://github.com/immortalserg/AdafruitGFXRusFonts
 *
 * Необходимо определить нужные шрифты и размеры, для включения их в прошивку
 * для этого можно скопировать отсюда нужные:
 * #define MWOS_WIDGET_FONT_*
 */

/*
#define MWOS_WIDGET_FONT_PT6 0
#define MWOS_WIDGET_FONT_PT7 1
#define MWOS_WIDGET_FONT_PT8 2
#define MWOS_WIDGET_FONT_PT9 3
#define MWOS_WIDGET_FONT_PT10 4
#define MWOS_WIDGET_FONT_PT12 5
#define MWOS_WIDGET_FONT_PT14 6
#define MWOS_WIDGET_FONT_PT16 7

#define MWOS_WIDGET_FONT_Bahamas 1
#define MWOS_WIDGET_FONT_Cooper 2
#define MWOS_WIDGET_FONT_CourierCyr 3
#define MWOS_WIDGET_FONT_Crystal 4
#define MWOS_WIDGET_FONT_CrystalNormal 5
#define MWOS_WIDGET_FONT_FreeMono 6
#define MWOS_WIDGET_FONT_FreeMonoBold 7
#define MWOS_WIDGET_FONT_FreeMonoBoldOblique 8
#define MWOS_WIDGET_FONT_FreeMonoOblique 9
#define MWOS_WIDGET_FONT_FreeSans 10
#define MWOS_WIDGET_FONT_FreeSansBold 11
#define MWOS_WIDGET_FONT_FreeSansBoldOblique 12
#define MWOS_WIDGET_FONT_FreeSansOblique 13
#define MWOS_WIDGET_FONT_FreeSerif 14
#define MWOS_WIDGET_FONT_FreeSerifBold 15
#define MWOS_WIDGET_FONT_FreeSerifBoldItalic 16
#define MWOS_WIDGET_FONT_FreeSerifItalic 17
#define MWOS_WIDGET_FONT_TimesNRCyr 18
 */

enum MWOSWidgetTextFontSize: uint8_t {
#ifdef MWOS_WIDGET_FONT_PT6
    pt6=0,
#endif
#ifdef MWOS_WIDGET_FONT_PT7
    pt7=1,
#endif
#ifdef MWOS_WIDGET_FONT_PT8
    pt8=2,
#endif
#ifdef MWOS_WIDGET_FONT_PT9
    pt9=3,
#endif
#ifdef MWOS_WIDGET_FONT_PT10
    pt10=4,
#endif
#ifdef MWOS_WIDGET_FONT_PT12
    pt12=5,
#endif
#ifdef MWOS_WIDGET_FONT_PT14
    pt14=6,
#endif
#ifdef MWOS_WIDGET_FONT_PT16
    pt16=7
#endif
};

// список размеров шрифтов, в формате JSON для параметра fontSize
const char * PROGMEM MWOS_WIDGET_FONT_SIZES_LIST="{'name':'fontSize','value_format':'"
#ifdef MWOS_WIDGET_FONT_PT6
                                                 "0=pt6;"
#endif
#ifdef MWOS_WIDGET_FONT_PT7
                                                 "1=pt7;"
#endif
#ifdef MWOS_WIDGET_FONT_PT8
                                                 "2=pt8;"
#endif
#ifdef MWOS_WIDGET_FONT_PT9
                                                 "3=pt9;"
#endif
#ifdef MWOS_WIDGET_FONT_PT10
                                                 "4=pt10;"
#endif
#ifdef MWOS_WIDGET_FONT_PT12
                                                 "5=pt12;"
#endif
#ifdef MWOS_WIDGET_FONT_PT14
                                                 "6=pt14;"
#endif
#ifdef MWOS_WIDGET_FONT_PT16
                                                 "7=pt16;"
#endif
                                                 "'}";

// список размеров шрифтов, в формате JSON для параметра fontSize
const char * PROGMEM MWOS_WIDGET_SELECTED_FONT_SIZES_LIST="{'name':'selectedFontSize','value_format':'"
#ifdef MWOS_WIDGET_FONT_PT6
"0=pt6;"
#endif
#ifdef MWOS_WIDGET_FONT_PT7
"1=pt7;"
#endif
#ifdef MWOS_WIDGET_FONT_PT8
"2=pt8;"
#endif
#ifdef MWOS_WIDGET_FONT_PT9
"3=pt9;"
#endif
#ifdef MWOS_WIDGET_FONT_PT10
"4=pt10;"
#endif
#ifdef MWOS_WIDGET_FONT_PT12
"5=pt12;"
#endif
#ifdef MWOS_WIDGET_FONT_PT14
"6=pt14;"
#endif
#ifdef MWOS_WIDGET_FONT_PT16
"7=pt16;"
#endif
"'}";

// список шрифтов, в формате JSON для параметра font
const char * PROGMEM MWOS_WIDGET_FONTS_LIST="{'name':'font','value_format':'0=Standart"
#ifdef MWOS_WIDGET_FONT_Bahamas
                                            ";1=RuBahamas"
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
                                            ";2=RuCooper"
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
                                            ";3=RuCourierCyr"
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
                                            ";4=RuCrystal"
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
                                            ";5=RuCrystalNormal"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
                                            ";6=RuFreeMono"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
                                            ";7=RuFreeMonoBold"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
                                            ";8=RuFreeMonoBoldOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
                                            ";9=RuFreeMonoOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
                                            ";10=RuFreeSans"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
                                            ";11=RuFreeSansBold"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
                                            ";12=RuFreeSansBoldOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
                                            ";13=RuFreeSansOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
                                            ";14=RuFreeSerif"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
                                            ";15=RuFreeSerifBold"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
                                            ";16=RuFreeSerifBoldItalic"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
                                            ";17=RuFreeSerifItalic"
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
                                            ";18=RuTimesNRCyr"
#endif
                                            "'}";

const char * PROGMEM MWOS_WIDGET_SELECTED_FONTS_LIST="{'name':'selectedFont','value_format':'0=Standart"
#ifdef MWOS_WIDGET_FONT_Bahamas
";1=RuBahamas"
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
";2=RuCooper"
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
";3=RuCourierCyr"
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
";4=RuCrystal"
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
";5=RuCrystalNormal"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
";6=RuFreeMono"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
";7=RuFreeMonoBold"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
";8=RuFreeMonoBoldOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
";9=RuFreeMonoOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
";10=RuFreeSans"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
";11=RuFreeSansBold"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
";12=RuFreeSansBoldOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
";13=RuFreeSansOblique"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
";14=RuFreeSerif"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
";15=RuFreeSerifBold"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
";16=RuFreeSerifBoldItalic"
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
";17=RuFreeSerifItalic"
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
";18=RuTimesNRCyr"
#endif
"'}";

enum MWOSWidgetTextFonts: uint8_t {
    Standart=0,
#ifdef MWOS_WIDGET_FONT_Bahamas
    RuBahamas=1,
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
    RuCooper=2,
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
    RuCourierCyr=3,
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
    RuCrystal=4,
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
    RuCrystalNormal=5,
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
    RuFreeMono=6,
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
    RuFreeMonoBold=7,
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
    RuFreeMonoBoldOblique=8,
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
    RuFreeMonoOblique=9,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
    RuFreeSans=10,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
    RuFreeSansBold=11,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
    RuFreeSansBoldOblique=12,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
    RuFreeSansOblique=13,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
    RuFreeSerif=14,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
    RuFreeSerifBold=15,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
    RuFreeSerifBoldItalic=16,
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
    RuFreeSerifItalic=17,
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
    RuTimesNRCyr=18,
#endif
};

#ifdef MWOS_WIDGET_FONT_PT6
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas6.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper6.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr6.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal6.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic6.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic6.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr6.h>
#endif
#endif //MWOS_WIDGET_FONT_PT6

#ifdef MWOS_WIDGET_FONT_PT7
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas7.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper7.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr7.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal7.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic7.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic7.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr7.h>
#endif
#endif //MWOS_WIDGET_FONT_PT7

#ifdef MWOS_WIDGET_FONT_PT8
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas8.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper8.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr8.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal8.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic8.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic8.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr8.h>
#endif
#endif //MWOS_WIDGET_FONT_PT8

#ifdef MWOS_WIDGET_FONT_PT9
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas9.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper9.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr9.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal9.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic9.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic9.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr9.h>
#endif
#endif //MWOS_WIDGET_FONT_PT9

#ifdef MWOS_WIDGET_FONT_PT10
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas10.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper10.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr10.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal10.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic10.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic10.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr10.h>
#endif
#endif //MWOS_WIDGET_FONT_PT10

#ifdef MWOS_WIDGET_FONT_PT12
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas12.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper12.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr12.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal12.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic12.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic12.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr12.h>
#endif
#endif //MWOS_WIDGET_FONT_PT12

#ifdef MWOS_WIDGET_FONT_PT14
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas14.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper14.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr14.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal14.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic14.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic14.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr14.h>
#endif
#endif //MWOS_WIDGET_FONT_PT14

#ifdef MWOS_WIDGET_FONT_PT16
#ifdef MWOS_WIDGET_FONT_Bahamas
#include <FontsRus/Bahamas16.h>
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
#include <FontsRus/Cooper16.h>
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
#include <FontsRus/CourierCyr16.h>
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
#include <FontsRus/Crystal16.h>
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
#include <FontsRus/CrystalNormal16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
#include <FontsRus/FreeMono16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
#include <FontsRus/FreeMonoBold16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
#include <FontsRus/FreeMonoBoldOblique16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
#include <FontsRus/FreeMonoOblique16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
#include <FontsRus/FreeSans16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
#include <FontsRus/FreeSansBold16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
#include <FontsRus/FreeSansBoldOblique16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
#include <FontsRus/FreeSansOblique16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
#include <FontsRus/FreeSerif16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
#include <FontsRus/FreeSerifBold16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
#include <FontsRus/FreeSerifBoldItalic16.h>
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
#include <FontsRus/FreeSerifItalic16.h>
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
#include <FontsRus/TimesNRCyr16.h>
#endif
#endif //MWOS_WIDGET_FONT_PT16

/**
 * Получить ссылку на шрифт
 * @param font  Тип шрифта
 * @param fontSize  Размер шрифта
 */
uint8_t * getFont(MWOSWidgetTextFonts font, MWOSWidgetTextFontSize fontSize) {
    uint8_t * f=NULL;
    switch (fontSize*100+font) {
#ifdef MWOS_WIDGET_FONT_PT6
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 1: return (uint8_t *) &Bahamas6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 2: return (uint8_t *) &Cooper6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 3: return (uint8_t *) &CourierCyr6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 4: return (uint8_t *) &Crystal6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 5: return (uint8_t *) &CrystalNormal6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 6: return (uint8_t *) &FreeMono6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 7: return (uint8_t *) &FreeMonoBold6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 8: return (uint8_t *) &FreeMonoBoldOblique6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 9: return (uint8_t *) &FreeMonoOblique6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 10:return (uint8_t *) &FreeSans6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 11:return (uint8_t *) &FreeSansBold6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 12:return (uint8_t *) &FreeSansBoldOblique6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 13:return (uint8_t *) &FreeSansOblique6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 14:return (uint8_t *) &FreeSerif6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 15:return (uint8_t *) &FreeSerifBold6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 16:return (uint8_t *) &FreeSerifBoldItalic6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 17:return (uint8_t *) &FreeSerifItalic6pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 18:return (uint8_t *) &TimesNRCyr6pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT6

#ifdef MWOS_WIDGET_FONT_PT7
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 101: return (uint8_t *) &Bahamas7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 102: return (uint8_t *) &Cooper7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 103: return (uint8_t *) &CourierCyr7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 104: return (uint8_t *) &Crystal7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 105: return (uint8_t *) &CrystalNormal7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 106: return (uint8_t *) &FreeMono7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 107: return (uint8_t *) &FreeMonoBold7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 108: return (uint8_t *) &FreeMonoBoldOblique7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 109: return (uint8_t *) &FreeMonoOblique7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 110:return (uint8_t *) &FreeSans7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 111:return (uint8_t *) &FreeSansBold7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 112:return (uint8_t *) &FreeSansBoldOblique7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 113:return (uint8_t *) &FreeSansOblique7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 114:return (uint8_t *) &FreeSerif7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 115:return (uint8_t *) &FreeSerifBold7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 116:return (uint8_t *) &FreeSerifBoldItalic7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 117:return (uint8_t *) &FreeSerifItalic7pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 118:return (uint8_t *) &TimesNRCyr7pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT7

#ifdef MWOS_WIDGET_FONT_PT8
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 201: return (uint8_t *) &Bahamas8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 202: return (uint8_t *) &Cooper8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 203: return (uint8_t *) &CourierCyr8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 204: return (uint8_t *) &Crystal8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 205: return (uint8_t *) &CrystalNormal8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 206: return (uint8_t *) &FreeMono8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 207: return (uint8_t *) &FreeMonoBold8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 208: return (uint8_t *) &FreeMonoBoldOblique8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 209: return (uint8_t *) &FreeMonoOblique8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 210:return (uint8_t *) &FreeSans8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 211:return (uint8_t *) &FreeSansBold8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 212:return (uint8_t *) &FreeSansBoldOblique8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 213:return (uint8_t *) &FreeSansOblique8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 214:return (uint8_t *) &FreeSerif8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 215:return (uint8_t *) &FreeSerifBold8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 216:return (uint8_t *) &FreeSerifBoldItalic8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 217:return (uint8_t *) &FreeSerifItalic8pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 218:return (uint8_t *) &TimesNRCyr8pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT8

#ifdef MWOS_WIDGET_FONT_PT9
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 301: return (uint8_t *) &Bahamas9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 302: return (uint8_t *) &Cooper9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 303: return (uint8_t *) &CourierCyr9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 304: return (uint8_t *) &Crystal9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 305: return (uint8_t *) &CrystalNormal9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 306: return (uint8_t *) &FreeMono9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 307: return (uint8_t *) &FreeMonoBold9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 308: return (uint8_t *) &FreeMonoBoldOblique9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 309: return (uint8_t *) &FreeMonoOblique9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 310:return (uint8_t *) &FreeSans9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 311:return (uint8_t *) &FreeSansBold9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 312:return (uint8_t *) &FreeSansBoldOblique9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 313:return (uint8_t *) &FreeSansOblique9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 314:return (uint8_t *) &FreeSerif9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 315:return (uint8_t *) &FreeSerifBold9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 316:return (uint8_t *) &FreeSerifBoldItalic9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 317:return (uint8_t *) &FreeSerifItalic9pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 318:return (uint8_t *) &TimesNRCyr9pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT9

#ifdef MWOS_WIDGET_FONT_PT10
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 401: return (uint8_t *) &Bahamas10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 402: return (uint8_t *) &Cooper10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 403: return (uint8_t *) &CourierCyr10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 404: return (uint8_t *) &Crystal10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 405: return (uint8_t *) &CrystalNormal10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 406: return (uint8_t *) &FreeMono10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 407: return (uint8_t *) &FreeMonoBold10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 408: return (uint8_t *) &FreeMonoBoldOblique10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 409: return (uint8_t *) &FreeMonoOblique10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 410:return (uint8_t *) &FreeSans10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 411:return (uint8_t *) &FreeSansBold10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 412:return (uint8_t *) &FreeSansBoldOblique10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 413:return (uint8_t *) &FreeSansOblique10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 414:return (uint8_t *) &FreeSerif10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 415:return (uint8_t *) &FreeSerifBold10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 416:return (uint8_t *) &FreeSerifBoldItalic10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 417:return (uint8_t *) &FreeSerifItalic10pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 418:return (uint8_t *) &TimesNRCyr10pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT10

#ifdef MWOS_WIDGET_FONT_PT12
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 501: return (uint8_t *) &Bahamas12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 502: return (uint8_t *) &Cooper12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 503: return (uint8_t *) &CourierCyr12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 504: return (uint8_t *) &Crystal12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 505: return (uint8_t *) &CrystalNormal12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 506: return (uint8_t *) &FreeMono12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 507: return (uint8_t *) &FreeMonoBold12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 508: return (uint8_t *) &FreeMonoBoldOblique12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 509: return (uint8_t *) &FreeMonoOblique12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 510:return (uint8_t *) &FreeSans12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 511:return (uint8_t *) &FreeSansBold12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 512:return (uint8_t *) &FreeSansBoldOblique12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 513:return (uint8_t *) &FreeSansOblique12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 514:return (uint8_t *) &FreeSerif12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 515:return (uint8_t *) &FreeSerifBold12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 516:return (uint8_t *) &FreeSerifBoldItalic12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 517:return (uint8_t *) &FreeSerifItalic12pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 518:return (uint8_t *) &TimesNRCyr12pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT12

#ifdef MWOS_WIDGET_FONT_PT14
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 601: return (uint8_t *) &Bahamas14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 602: return (uint8_t *) &Cooper14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 603: return (uint8_t *) &CourierCyr14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 604: return (uint8_t *) &Crystal14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 605: return (uint8_t *) &CrystalNormal14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 606: return (uint8_t *) &FreeMono14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 607: return (uint8_t *) &FreeMonoBold14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 608: return (uint8_t *) &FreeMonoBoldOblique14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 609: return (uint8_t *) &FreeMonoOblique14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 610:return (uint8_t *) &FreeSans14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 611:return (uint8_t *) &FreeSansBold14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 612:return (uint8_t *) &FreeSansBoldOblique14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 613:return (uint8_t *) &FreeSansOblique14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 614:return (uint8_t *) &FreeSerif14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 615:return (uint8_t *) &FreeSerifBold14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 616:return (uint8_t *) &FreeSerifBoldItalic14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 617:return (uint8_t *) &FreeSerifItalic14pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 618:return (uint8_t *) &TimesNRCyr14pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT14

#ifdef MWOS_WIDGET_FONT_PT16
#ifdef MWOS_WIDGET_FONT_Bahamas
        case 701: return (uint8_t *) &Bahamas16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Cooper
        case 702: return (uint8_t *) &Cooper16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CourierCyr
        case 703: return (uint8_t *) &CourierCyr16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_Crystal
        case 704: return (uint8_t *) &Crystal16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_CrystalNormal
        case 705: return (uint8_t *) &CrystalNormal16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMono
        case 706: return (uint8_t *) &FreeMono16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBold
        case 707: return (uint8_t *) &FreeMonoBold16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoBoldOblique
        case 708: return (uint8_t *) &FreeMonoBoldOblique16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeMonoOblique
        case 709: return (uint8_t *) &FreeMonoOblique16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSans
        case 710:return (uint8_t *) &FreeSans16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBold
        case 711:return (uint8_t *) &FreeSansBold16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansBoldOblique
        case 712:return (uint8_t *) &FreeSansBoldOblique16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSansOblique
        case 713:return (uint8_t *) &FreeSansOblique16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerif
        case 714:return (uint8_t *) &FreeSerif16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBold
        case 715:return (uint8_t *) &FreeSerifBold16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifBoldItalic
        case 716:return (uint8_t *) &FreeSerifBoldItalic16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_FreeSerifItalic
        case 717:return (uint8_t *) &FreeSerifItalic16pt8b;
#endif
#ifdef MWOS_WIDGET_FONT_TimesNRCyr
        case 718:return (uint8_t *) &TimesNRCyr16pt8b;
#endif
#endif // #ifdef MWOS_WIDGET_FONT_PT16
        
    }
    return NULL;
}
