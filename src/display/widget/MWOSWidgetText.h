#ifndef MWOS3_ESP32S3_MWOSWIDGETTEXT_H
#define MWOS3_ESP32S3_MWOSWIDGETTEXT_H

#include "MWOSWidget.h"
#include "MWOSFonts.h"

#ifndef MWOS_STORAGE_WIDGET_TEXT
#define MWOS_STORAGE_WIDGET_TEXT MWOS_STORAGE_NVS
#endif

/**
 * Виджет показывает текст на экране
 * сам текст хранится в EEPROM и может быть изменен удаленно
 * так же, позволяет задавать шрифт текста
 *
 * Доп.шрифты тут:
 * https://github.com/immortalserg/AdafruitGFXRusFonts
 *
 * @tparam stringLength    Максимальный размер строки
 */
template<MWOS_PARAM_INDEX_UINT stringLength>
class MWOSWidgetText: public MWOSWidget {
public:

    String _text="";
    uint8_t _font=0;
    uint8_t _fontSize=0;
    uint8_t _textSize=0;
    int8_t _colorCount=-1; // через сколько символов задать цвет по умолчанию

    // тест для показа на дисплее (до stringLength символов)
    MWOS_PARAM(10, text, mwos_param_string, mwos_param_option, MWOS_STORAGE_WIDGET_TEXT, stringLength);
    // шрифт тип
    MWOS_PARAM_F(11, font, mwos_param_bits7, mwos_param_option, MWOS_STORAGE_EEPROM, 1, MWOS_WIDGET_FONTS_LIST);
    // шрифт размер
    MWOS_PARAM_F(12, fontSize, mwos_param_bits5, mwos_param_option, MWOS_STORAGE_EEPROM, 1, MWOS_WIDGET_FONT_SIZES_LIST);
    // множитель размера текста + 1
    MWOS_PARAM_F(13, textSize, mwos_param_bits3, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "{'name':'textSize','value_format':'x1;x2;x3;x4;x5;x6;x7;x8'}");

    MWOSWidgetText(MWOSDisplay * _displayModule) : MWOSWidget(_displayModule,(char *) F("widgetText")) {
        AddParam(&p_text);
        AddParam(&p_font);
        AddParam(&p_fontSize);
        AddParam(&p_textSize);
    }

    /***
    * Вызывается на многие системные события и каждый тик операционной системы
    * @param modeEvent    Тип вызываемого системного события
    */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSWidget::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _text = MWOSWidget::loadValueString(_text, &p_text);
            _font = MWOSWidget::loadValue(_font, &p_font);
            _fontSize = MWOSWidget::loadValue(_fontSize, &p_fontSize);
            _textSize = MWOSWidget::loadValue(_textSize, &p_textSize);
        }
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id==10) {
            String txt=receiverDat->GetValueString();
            print(txt);
            MWOSWidget::saveValueStr(txt,&p_text);
        } else
            MWOSWidget::onReceiveValue(receiverDat);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 10: {
                if (arrayIndex>=_text.length()) return 0; // дополняем текст справа нулями до заданной длины
                return _text[arrayIndex];
            } break;
            case 11: return _font;
            case 12: return _fontSize;
            case 13: return _textSize;
        }
        return MWOSWidget::getValue(param, arrayIndex);
    }

    /**
     * Задать текст не изменяя его параметров
     * @param text
     */
    void setText(const String &text) {
        _text=text;
    }

    /**
     * Задать параметры текста
     * @param text      Сам текст
     * @param font      Шрифт
     * @param fontSize  Размер шрифта
     * @param textSize  Номер масштаба теста (0-1x1,1-x2,2-x3,3-x4,...)
     */
    void setText(const String &text, MWOSWidgetTextFonts font,MWOSWidgetTextFontSize fontSize, uint8_t textSize=0) {
        _text=text;
        _font=(uint8_t) font;
        _fontSize=(uint8_t) fontSize;
        _textSize=textSize;
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        if (!IsVisible()) return;
        if (_displayUpdate==0) displayModule->fillRect(_x,_y,_width,_height,displayModule->_backColor); // сотрем область виджета
        //displayModule->display->setTextSize(1);
        if (_text!="") setFont();
        //displayModule->display->setTextWrap(false);
        printNow(_text,_x,_y,_width,_height,_color);
        MWOSWidget::print();
    }

    /**
     * Печатает символ. Умеет печатать спецсимволы.
     * @param ch
     * @param posX
     * @param posY
     * @param charHeight Возвращает высоту символа
     * @param xMin  Минимальная позиция по X
     * @param xOffset  Промежуток по X между символами
     * @param printNow  Печатать символ (иначе - просто подсчитывает его размер)
     * @return  Ширина символа
     */
    uint16_t printChar(char ch, int16_t *posX, int16_t *posY, uint16_t *charHeight, int16_t xOffset, bool printNow) {
        int16_t x=*posX;
        int16_t y=*posY;
        uint16_t chW=0;
        uint16_t chH=0;
        //MW_LOG('~'); MW_LOG(ch); MW_LOG('='); MW_LOG((uint16_t) ch,HEX); MW_LOG('~');
        if ((ch>=0x20 && ch<0xd0) || ch==13 || ch==10) { // это обычный символ
            String str=String(ch);
            displayModule->getTextBounds(str,*posX,*posY,&x,&y,&chW,&chH);

            if (*charHeight < chH) *charHeight=chH;
            if (printNow) {
                //displayModule->setCursor(x,y);
                displayModule->print(ch);
                if (_colorCount>=0) {
                    _colorCount--;
                    if (_colorCount==0) {
                        displayModule->setTextColor(_color);
                    }
                }
            }
        } else { // это спецсимволы
            String str="0";
            displayModule->getTextBounds(str,*posX,*posY,&x,&y,&chW,&chH);
            uint16_t rSize=chW-2;
            int16_t rX= x+1+xOffset;
            int16_t rY= y + (chH-rSize)/2;
            if (*charHeight < chH) *charHeight=chH;
            switch (ch) {
                case 0x10: { // стрелка вправо UpCenter,DownCenter,CenterLeft,CenterRight
                    if (printNow) displayModule->fillTriangleArrow(rX,rY,rSize,rSize,CenterRight,_color);
                } break;
                case 0x11: { // стрелка влево
                    if (printNow) displayModule->fillTriangleArrow(rX,rY,rSize,rSize,CenterLeft,_color);
                } break;
                case 0x1F: { // стрелка вверх
                    if (printNow) displayModule->fillTriangleArrow(rX,rY,rSize,rSize,UpCenter,_color);
                } break;
                case 0x1E: { // стрелка вниз
                    if (printNow) displayModule->fillTriangleArrow(rX,rY,rSize,rSize,DownCenter,_color);
                } break;
                default: {
                    chW=printSuperChar(ch,x+xOffset,y,chW,chH, printNow);
                }
            }
            if (printNow && chW>0) displayModule->setCursor(*posX+chW+xOffset, *posY);
        }
        if (chW==0) return 0;
        *posX+=chW+xOffset;
        return chW+xOffset;
    }

    /**
     * Напечатать спецсимвол (1-31)
     * @param ch
     * @param posX
     * @param posY
     * @param chW
     * @param chH
      * @param printNow Напечатать (или только вернуть ширину)
     * @return  Ширина напечатанного спецсимвола
     */
    virtual uint16_t printSuperChar(char ch, int16_t posX, int16_t posY, uint16_t chW, uint16_t chH, bool printNow) {
        return 0;
    }

    /**
     * Отрисовать текст (с выравниванием) на экране немедленно
     * @param text
     * @param posX
     * @param posY
     * @param textWidth
     * @param textHeight
     * @param textColor
     */
    void printNow(const String &text, int16_t posX, int16_t posY, uint16_t textWidth, uint16_t textHeight, mwos_color textColor) {
        if (text=="") return;
        //if (text.indexOf(13)<0) text.replace(10,13);
        //else text.replace("\u000a","");
        int16_t x,y;
        uint16_t w,h;
        displayModule->setTextSize(_textSize+1);
        displayModule->setTextColor(textColor);
        if (text.length()>0) {
            // учтем смещение символа
            displayModule->getTextBounds("0",40,40,&x,&y,&w,&h);
            int16_t dX=40-x;
            int16_t dY=40-y;
            int16_t chW=w;
            displayModule->getTextBounds("00",40,40,&x,&y,&w,&h);
            int16_t xOffset=w-chW*2; // пробел между символами
            //MW_LOG_MODULE(this); MW_LOG(F("printNow"));  MW_LOG(F("; x=")); MW_LOG(posX); MW_LOG(F("; y=")); MW_LOG(posY); MW_LOG(F("; tW=")); MW_LOG(textWidth); MW_LOG(F("; tH=")); MW_LOG(textHeight); MW_LOG(F("; h=")); MW_LOG(h); MW_LOG(F("; xOff=")); MW_LOG_LN(xOffset);
            w=0;
            h=0;
            x=posX;
            y=posY;
            uint16_t wMax=0;
            for (int i = 0; i < text.length(); ++i) {
                char ch=text.charAt(i);
                int16_t xMin=x;
                w += printChar(ch, &x, &y, &h, xOffset, false);
                if ((ch==13 || ch==10) || xMin>x) {
                    if (wMax<w) wMax=w;
                    w=0;
                }
            }
            if (wMax>w) w=wMax;
            w-=xOffset; // вычтем ширину последнего пробела между символами
            x= alignX(posX, textWidth, w)+dX;
            y= alignY(posY, textHeight, h)+dY;
            //MW_LOG_MODULE(this); MW_LOG(F("printNow: ")); MW_LOG(text); MW_LOG(F("; x=")); MW_LOG(x); MW_LOG(F("; y=")); MW_LOG(y); MW_LOG(F("; w=")); MW_LOG(w); MW_LOG(F("; h=")); MW_LOG(h); MW_LOG(F("; dX=")); MW_LOG(dX); MW_LOG(F("; dY=")); MW_LOG_LN(dY);
            displayModule->setCursor(x, y);
            for (int i = 0; i < text.length(); ++i) {
                char ch=text.charAt(i);
                printChar(ch, &x, &y, &h, xOffset,true);
            }
        }
    }

    void setFontNew(uint8_t _fontNew, uint8_t _fontSizeNew) {
        displayModule->setFont(getFont((MWOSWidgetTextFonts) _fontNew,(MWOSWidgetTextFontSize) _fontSizeNew));
    }

    void setFont() {
        displayModule->setFont(getFont((MWOSWidgetTextFonts) _font,(MWOSWidgetTextFontSize) _fontSize));
    }

    /**
     * Показать тект на дисплее
     * @param text
     */
    void print(const String &text) {
        _text=text;
        SetParamChanged(&p_text,0,true);
        if (IsVisible()) SetUpdate();
        MW_LOG_MODULE(this); MW_LOG(F("print text: ")); MW_LOG_LN(_text);
    }

    void printString(const String &text) {
        print(text);
    }

private:


};


#endif //MWOS3_ESP32S3_MWOSWIDGETTEXT_H
