#ifndef MWOS3_ESP32S3_MWOSWidgetMenu_H
#define MWOS3_ESP32S3_MWOSWidgetMenu_H
/**
 * Виджет вертикального текстового меню (без скролинга)
 * В тексте задаются пункты меню через точку с запятой
 * На экране пункты выводятся вертикальным списком
 * Используются 3 клавиши - "вверх", "вниз" и "выбор" (опционально - "назад")
 * Выбор элемента меню отражается в параметре и меню становится неактивным
 *
 * Если задано родительское меню, то текущее меню делает свой виртуальный экран активным,
 * когда в родительском меню выбрали его номер
 *
 * Ежемоментно может быть активным только одно меню
 *
 * Можно задать пустое меню с кнопкой "назад".
 * Тогда оно будет активировать свой виртуальный экран при выборе строки в родительском меню
 * и возвращаться в родительское меню по кнопке "назад"
 *
 */
#include "MWOSWidgetText.h"
#include "core/MWOSKeyboardBase.h"

template<MWOS_PARAM_INDEX_UINT stringLength>
class MWOSWidgetMenu: public MWOSWidgetText<stringLength> {
public:

#pragma pack(push,1)
    uint16_t _cursorWidth=1000;
    uint16_t _cursorHeight=16;
    mwos_color _cursorBackColor=MWOS_COLOR_MAIN;
    mwos_color _cursorTextColor=MWOS_COLOR_BACK;
    uint8_t _cursorY=0;
    uint8_t _lineHeight=16;
    uint8_t _parentNum=255; // номер строки родительского меню (selectedValue), для выбора этого меню
    uint8_t _selected=0; // Выбранная (клавишей "выбор") строка меню (cursor + 1)
    uint8_t _selectedLine=255; // Выбранная срока (дла выделения шрифтом) 255-нет
    uint8_t _cursor=0; // Текущий номер строка меню (позиция курсора от 0)
    uint8_t lines=0; // количество линий меню
    uint8_t _ring=1;
    uint8_t _selectedFont=255;
    uint8_t _selectedFontSize=0;
    mwos_color _selectedTextColor=MWOS_COLOR_BLACK;
#pragma pack(pop)
    MWOSKeyboardBase * _keyboard;
    MWOSWidget * _parentMenu=NULL;

    // Выбранная (клавишей "выбор") строка меню (0-нет,1-верхняя)
    MWOS_PARAM(20, selected, mwos_param_uint8, mwos_param_control, MWOS_STORAGE_NO, 1);
    // Текущий номер строка меню (позиция курсора от 0)
    MWOS_PARAM(21, cursor, mwos_param_uint8, mwos_param_control, MWOS_STORAGE_NO, 1);
    // размер курсора меню - ширина
    MWOS_PARAM(22, cursorWidth, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // размер курсора меню - высота
    MWOS_PARAM(23, cursorHeight, mwos_param_uint16, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // цвет фона курсора
    MWOS_PARAM(24, cursorBackColor, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // цвет текста курсора
    MWOS_PARAM(25, cursorTextColor, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // позиция Y курсора, относительно строки
    MWOS_PARAM(26, cursorY, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // высота одного пункта меню (строки меню располагаются через эту высоту)
    MWOS_PARAM(27, lineHeight, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // id модуля родительского меню
    MWOS_PARAM(28, parentMenuId, MWOS_PARAM_INT_PTYPE, mwos_param_option+mwos_param_module_id, MWOS_STORAGE_EEPROM, 1);
    // номер этого меню в родительском меню (больше 0)
    MWOS_PARAM(29, parentNum, mwos_param_uint8, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // закольцевать это меню
    MWOS_PARAM(30, ring, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    // Выбранное значение - шрифт тип
    MWOS_PARAM_F(31, selectedFont, mwos_param_bits7, mwos_param_option, MWOS_STORAGE_EEPROM, 1, MWOS_WIDGET_SELECTED_FONTS_LIST);
    // Выбранное значение - шрифт размер
    MWOS_PARAM_F(32, selectedFontSize, mwos_param_bits5, mwos_param_option, MWOS_STORAGE_EEPROM, 1, MWOS_WIDGET_SELECTED_FONT_SIZES_LIST);
    // Выбранное значение - цвет текста
    MWOS_PARAM(33, selectedTextColor, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    MWOSWidgetMenu(MWOSDisplay * _displayModule, MWOSKeyboardBase * keyboard) : MWOSWidgetText<stringLength>(_displayModule) {
        MWOSWidgetText<stringLength>::name=(char *) F("menuMain");
        MWOSWidgetText<stringLength>::AddParam(&p_selected);
        MWOSWidgetText<stringLength>::AddParam(&p_cursor);
        MWOSWidgetText<stringLength>::AddParam(&p_cursorWidth);
        MWOSWidgetText<stringLength>::AddParam(&p_cursorHeight);
        MWOSWidgetText<stringLength>::AddParam(&p_cursorBackColor);
        MWOSWidgetText<stringLength>::AddParam(&p_cursorTextColor);
        MWOSWidgetText<stringLength>::AddParam(&p_cursorY);
        MWOSWidgetText<stringLength>::AddParam(&p_lineHeight);
        MWOSWidgetText<stringLength>::AddParam(&p_parentMenuId);
        MWOSWidgetText<stringLength>::AddParam(&p_parentNum);
        MWOSWidgetText<stringLength>::AddParam(&p_ring);
        MWOSWidgetText<stringLength>::AddParam(&p_selectedFont);
        MWOSWidgetText<stringLength>::AddParam(&p_selectedFontSize);
        MWOSWidgetText<stringLength>::AddParam(&p_selectedTextColor);
        _keyboard=keyboard;
    }

    MWOSWidgetMenu(MWOSDisplay * _displayModule, MWOSKeyboardBase * keyboard, MWOSWidget * parentMenu, uint8_t parentNum) : MWOSWidgetMenu(_displayModule, keyboard) {
        _parentMenu=parentMenu;
        _parentNum=parentNum;
    }

    MWOSWidgetMenu(MWOSDisplay * _displayModule, MWOSKeyboardBase * keyboard, const String &textMenu) : MWOSWidgetMenu(_displayModule, keyboard) {
        MWOSWidgetText<stringLength>::_text=textMenu;
    }

    /**
     * Установить выбранную строку (0-сбросить выбор)
     * @param lineNum
     */
    void SetSelected(uint8_t lineNum) {
        _selected=lineNum;
        MWOSWidgetText<stringLength>::SetParamChanged(&p_selected);
        MWOSWidgetText<stringLength>::SetUpdate();
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSWidgetText<stringLength>::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _cursorWidth = MWOSWidget::loadValue(_cursorWidth, &p_cursorWidth, 0);
            if (_cursorWidth>MWOSWidget::displayModule->width()) _cursorWidth=MWOSWidget::displayModule->width();
            _cursorHeight = MWOSWidget::loadValue(_cursorHeight, &p_cursorHeight, 0);
            _cursorBackColor = MWOSWidget::loadValue(_cursorBackColor, &p_cursorBackColor, 0);
            _cursorTextColor = MWOSWidget::loadValue(_cursorTextColor, &p_cursorTextColor, 0);
            _cursorY = MWOSWidget::loadValue(_cursorY, &p_cursorY, 0);
            _lineHeight = MWOSWidget::loadValue(_lineHeight, &p_lineHeight, 0);
            _parentNum = MWOSWidget::loadValue(_parentNum, &p_parentNum, 0);
            _ring = MWOSWidget::loadValue(_ring, &p_ring, 0);
            MWOSWidget::loadLinkModule(_parentMenu, MODULE_UNDEFINED, &p_parentMenuId, 0);
            // определим область видимости меню
            maxLines=1; if (_lineHeight>0) maxLines=MWOSWidgetText<stringLength>::_height / _lineHeight;
            if (maxLines<1) maxLines=1;
            fromLine=_cursor-maxLines+1;
            if (fromLine<0) fromLine=0;
            toLine=_cursor+maxLines-1;
            _selectedFont = MWOSWidget::loadValue(_selectedFont, &p_selectedFont, 0);
            if (_selectedFont==255) {
                _selectedFont = MWOSWidgetText<stringLength>::_font;
                _selectedFontSize = MWOSWidgetText<stringLength>::_fontSize;
                _selectedTextColor = MWOSWidget::_color;
            } else {
                _selectedFontSize = MWOSWidget::loadValue(_selectedFontSize, &p_selectedFontSize, 0);
                _selectedTextColor = MWOSWidget::loadValue(_selectedTextColor, &p_selectedTextColor, 0);
            }
            MW_LOG_MODULE(this); MW_LOG(F("menu set: ")); MW_LOG(MWOSWidgetText<stringLength>::_height); MW_LOG('/'); MW_LOG(_lineHeight); MW_LOG('='); MW_LOG(maxLines); MW_LOG('='); MW_LOG(fromLine); MW_LOG('-'); MW_LOG_LN(toLine);
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (_parentMenu!=NULL && _parentMenu->IsVisible()) {
                if (_parentMenu->getValueByParamId(20,0)==_parentNum) {
                    _parentMenu->setValueByParamId(0,20,0);
                    MWOSWidgetText<stringLength>::SetVisible();
                }
            }
            if (!MWOSWidgetText<stringLength>::IsVisible()) return;
            uint8_t pressKey=_keyboard->GetPressedKey();
            switch (pressKey) {
                case MW_KEY_UP: {
                    if (lines==0) _cursor=0;
                    else
                    if (_cursor==0) {
                        if (_ring) _cursor=lines-1;
                    } else _cursor--;
                    MWOSWidgetText<stringLength>::SetParamChanged(&p_cursor,0,true);
                    MWOSWidgetText<stringLength>::SetUpdate();
                } break;
                case MW_KEY_DOWN: {
                    if (lines==0) _cursor=0;
                    else {
                        _cursor++;
                        if (_cursor>=lines) {
                            if (_ring) _cursor=0;
                            else _cursor=lines-1;
                        }
                    }
                    MWOSWidgetText<stringLength>::SetParamChanged(&p_cursor,0,true);
                    MWOSWidgetText<stringLength>::SetUpdate();
                } break;
                case MW_KEY_OK: {
                    SetSelected(_cursor+1);
                    _keyboard->RemoveKeys(); // удалим код из буфера, что-бы другие подменю его не поймали
                    MWOSWidget::displayModule->SetDisplayUpdate();
                } break;
                case MW_KEY_ESC:
                case MW_KEY_EXIT: {
                    if (_parentMenu!=NULL) {
                        // вернемся к родительскому меню
                        _keyboard->RemoveKeys(); // удалим код из буфера, что-бы другие подменю его не поймали
                        _parentMenu->setValueByParamId(0,20,0);
                        _parentMenu->SetVisible();
                        _parentMenu->SetUpdate();
                    }
                } break;
            }
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 20: return selectedValue();
            case 21: return _cursor;
            case 22: return _cursorWidth;
            case 23: return _cursorHeight;
            case 24: return _cursorBackColor;
            case 25: return _cursorTextColor;
            case 26: return _cursorY;
            case 27: return _lineHeight;
            case 28: {
                if (_parentMenu==NULL) return -1;
                return _parentMenu->id;
            }
            case 29: return _parentNum;
            case 30: return _ring;
            case 31: return _selectedFont;
            case 32: return _selectedFontSize;
            case 33: return _selectedTextColor;
        }
        return MWOSWidgetText<stringLength>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        if (!MWOSWidgetText<stringLength>::IsVisible() || MWOSWidgetText<stringLength>::_text.length()==0) return;
        if (MWOSWidgetText<stringLength>::_displayUpdate==0) MWOSWidgetText<stringLength>::displayModule->fillRect(
                    MWOSWidgetText<stringLength>::_x,MWOSWidgetText<stringLength>::_y,
                    MWOSWidgetText<stringLength>::_width,MWOSWidgetText<stringLength>::_height,
                    MWOSWidgetText<stringLength>::displayModule->_backColor); // сотрем область виджета
        // определим область видимости меню
        if (_cursor>toLine) { // область видимости - вниз по списку
            toLine=_cursor;
            fromLine=toLine-maxLines+1;
            if (fromLine<0) fromLine=0;
        }
        if (_cursor<fromLine) { // область видимости - вверх по списку
            fromLine=_cursor;
            toLine=fromLine+maxLines-1;
        }
        // отрисуем курсор
        uint16_t posY=(_cursor-fromLine) * _lineHeight + MWOSWidgetText<stringLength>::_y;
        MWOSWidgetText<stringLength>::displayModule->fillRect(MWOSWidgetText<stringLength>::_x,posY,_cursorWidth,_cursorHeight,_cursorBackColor);
        MW_LOG_MODULE(this); MW_LOG(F("menu ")); MW_LOG(fromLine); MW_LOG('-'); MW_LOG(toLine); MW_LOG('='); MW_LOG_LN(_selectedLine);
        // выведем тест построчно
        MWOSWidgetText<stringLength>::setFont();
        lines=0;
        int16_t pos=0;
        while (pos>=0) {
            int16_t toPos=MWOSWidgetText<stringLength>::_text.indexOf(';',pos);
            String str;
            if (toPos>pos) {
                str=MWOSWidgetText<stringLength>::_text.substring(pos,toPos);
                pos=toPos+1;
            } else {
                str=MWOSWidgetText<stringLength>::_text.substring(pos);
                pos=-1;
            }
            if (lines>=fromLine && lines<=toLine) { // эта строка в пределах видимости
                posY=(lines-fromLine) * _lineHeight + MWOSWidgetText<stringLength>::_y;
                if (lines==_selectedLine) {
                    MWOSWidgetText<stringLength>::setFontNew(_selectedFont, _selectedFontSize);
                    changedFont=true;
                } else
                if (changedFont) {
                    MWOSWidgetText<stringLength>::setFont();
                    changedFont=false;
                }
                mwos_color color=MWOSWidgetText<stringLength>::_color;
                if (lines==_cursor) color=_cursorTextColor;
                else if (lines==_selectedLine) color=_selectedTextColor;
                MWOSWidgetText<stringLength>::printNow(onlyText(str),MWOSWidgetText<stringLength>::_x,posY,MWOSWidgetText<stringLength>::_width,_lineHeight,color);
            }
            lines++;
        }
        MWOSWidget::print();
    }
    bool changedFont=false;

    /**
     * Вернуть текст строки меню
     * @param lineNum   Строка меню
     * @return  Текст строки
     */
    String lineText(uint8_t lineNum) {
        int16_t lin=0;
        int16_t pos=0;
        while (pos>=0) {
            int16_t toPos=MWOSWidgetText<stringLength>::_text.indexOf(';',pos);
            String str;
            if (toPos>pos) {
                str=MWOSWidgetText<stringLength>::_text.substring(pos,toPos);
                pos=toPos+1;
            } else {
                str=MWOSWidgetText<stringLength>::_text.substring(pos);
                pos=-1;
            }
            if (lin==lineNum) { // эта строка в пределах видимости
                return str;
            }
            lin++;
        }
        return "";
    }


    /**
     * Вернуть текст меню выбранной строки
     * @return
     */
    String selectedText() {
        if (_selected<1) return "";
        return onlyText(lineText(_selected-1));
    }

    /**
     * Вернуть значение меню выбранной строки. Если значение не задано явно, то это номер выбранной строки от 0 (_selected-1).
     * @return
     */
    long selectedValue(int32_t defValue=-1) {
        if (_selected<1) return -1;
        return valueText(lineText(_selected-1),_selected-1);
    }

    /**
     * Выбрать строку меню, что-бы она помечалась как выбранная
     * @param lineNum
     */
    void selectLine(uint8_t lineNum) {
        if (lineNum<255) _cursor=lineNum;
        else _cursor=0;
        _selectedLine=lineNum;
    }

    /**
     * Возвращает строку, после разделителя двоеточия ':'. До двоеточия считается значением строки, а после - самой строкой.
     * @param stringWithValue
     * @return
     */
    String onlyText(const String &stringWithValue) {
        int16_t toPos=stringWithValue.indexOf(':');
        if (toPos<1) return stringWithValue;
        return stringWithValue.substring(toPos+1);
    }

    /**
     * Возвращает числовое значение из строки, до разделителя двоеточия ':'. До двоеточия считается значением строки, а после - самой строкой.
     * @param stringWithValue
     * @param defValue  Что вернуть, если разделитель не задан
     * @return
     */
    long valueText(const String &stringWithValue, int32_t defValue=0) {
        int16_t toPos=stringWithValue.indexOf(':');
        if (toPos<1) return defValue;
        return stringWithValue.substring(0,toPos).toInt();
    }

    void setParentMenu(MWOSWidgetMenu * parentMenu, uint8_t parentNum) {
        _parentMenu=parentMenu;
        _parentNum=parentNum;
    }

    void setDefaultLineHeight(uint8_t newLineHeight) {
        _cursorHeight=newLineHeight;
        _lineHeight=newLineHeight;
    }

    void setDefaultSelectedFont(MWOSWidgetTextFonts font,MWOSWidgetTextFontSize fontSize, mwos_color textColor=MWOS_COLOR_BLACK) {
        _selectedFont=(uint8_t) font;
        _selectedFontSize=(uint8_t) fontSize;
        _selectedTextColor=textColor;
    }

protected:
    int16_t maxLines=0;
    int16_t fromLine=0;
    int16_t toLine=0;

};


#endif //MWOS3_ESP32S3_MWOSWidgetMenu_H
