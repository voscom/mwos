#ifndef MWOS3_ESP32S3_MWOSWidgetPinCode_H
#define MWOS3_ESP32S3_MWOSWidgetPinCode_H
/*
 * Виджет текстового значения
 * добавляет текстовое значение к тексту
 * если в тексте есть макрос %0 - вставляет значение вместо макроса
 */
#include "MWOSWidgetTextValue.h"
#include "core/MWOSKeyboardBase.h"

enum MWOSPinCodeSelectType: uint8_t {
    none,
    arrow
};

/**
 * Виджет показывает на экране ввод цифрового кода стрелочками
 * Размер пинкода и базовое значение задается параметром _text
 * @tparam stringLength    Максимальный размер строки и значения
 */
template<MWOS_PARAM_INDEX_UINT stringLength>
class MWOSWidgetPinCode: public MWOSWidgetTextValue<stringLength> {
public:
    MWOSKeyboardBase * _keyboard;
    mwos_color _selectColor=MWOS_COLOR_BLUE;
    MWOSPinCodeSelectType _selectType=arrow;

    // цвет выделенного символа
    MWOS_PARAM(21, selectColor, mwos_param_color, mwos_param_option, MWOS_STORAGE_EEPROM, 1);
    // тип выделения
    MWOS_PARAM_F(22, selectType, mwos_param_bits4, mwos_param_option, MWOS_STORAGE_EEPROM, 1, "{'name':'selectType','value_format':'none;arrow'}");

    MWOSWidgetPinCode(MWOSDisplay * _displayModule, MWOSKeyboardBase * keyboard) : MWOSWidgetTextValue<stringLength>(_displayModule) {
        MWOSWidgetTextValue<stringLength>::name=(char *) F("widgetPinCode");
        MWOSWidgetText<stringLength>::AddParam(&p_selectColor);
        MWOSWidgetText<stringLength>::AddParam(&p_selectType);
        _keyboard=keyboard;
        StartNewPinCode();
    }

    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSWidgetTextValue<stringLength>::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _selectColor = MWOSWidget::loadValue(_selectColor, &p_selectColor);
            _selectType = (MWOSPinCodeSelectType) MWOSWidget::loadValue((uint8_t) _selectType, &p_selectType);
        } else
        if (modeEvent==EVENT_UPDATE) {
            if (!MWOSWidgetTextValue<stringLength>::IsVisible() ||
                MWOSWidgetTextValue<stringLength>::_value.isEmpty())
                return;
            uint8_t pressKey = _keyboard->GetPressedKey();
            if (pressKey == 0) return;
            switch (pressKey) {
                case MW_KEY_RIGHT:
                case MW_KEY_OK: {
                    if (pos + 1 >= MWOSWidgetTextValue<stringLength>::_value.length())
                        for (uint16_t i = 0; i < MWOSWidgetTextValue<stringLength>::_value.length(); ++i) {
                            char ch = MWOSWidgetTextValue<stringLength>::_value[i];
                            if (ch < '0' || ch > '9') {
                                pos = i - 1;
                            }
                        }
                    if (pos < MWOSWidgetTextValue<stringLength>::_value.length()) pos++;
                    if (isEnter()) MWOSWidget::SetParamChangedByParamId(20);
                } break;
                case MW_KEY_LEFT: {
                    if (pos > 0) pos--;
                }
                    break;
                case MW_KEY_UP: {
                    if (isEnter()) return;
                    MWOSWidgetTextValue<stringLength>::_value[pos]++;
                    if (MWOSWidgetTextValue<stringLength>::_value[pos] > '9' ||
                        MWOSWidgetTextValue<stringLength>::_value[pos] < '0')
                        MWOSWidgetTextValue<stringLength>::_value[pos] = '0';
                }
                    break;
                case MW_KEY_DOWN: {
                    if (isEnter()) return;
                    MWOSWidgetTextValue<stringLength>::_value[pos]--;
                    if (MWOSWidgetTextValue<stringLength>::_value[pos] > '9' ||
                        MWOSWidgetTextValue<stringLength>::_value[pos] < '0')
                        MWOSWidgetTextValue<stringLength>::_value[pos] = '9';
                }
                    break;
            }
            // покажем новый символ
            MWOSWidgetTextValue<stringLength>::show();
        }
    }

    /**
     * Показываем только значение
     */
    virtual void print() {
        if (!MWOSWidgetText<stringLength>::IsVisible()) return;
        String nowStr=MWOSWidgetText<stringLength>::_text;
        if (isEnter()) MWOSWidgetText<stringLength>::_text=MWOSWidgetTextValue<stringLength>::_value;
        else {
            if (pos<1) MWOSWidgetText<stringLength>::_text="";
            else MWOSWidgetText<stringLength>::_text=MWOSWidgetTextValue<stringLength>::_value.substring(0,pos);
            MWOSWidgetText<stringLength>::_text+="\u0002"+MWOSWidgetTextValue<stringLength>::_value.substring(pos); // вставим спецсимвол с кодом=2 -признак текущей позиции ввода
        }
        MWOSWidgetText<stringLength>::print();
        MWOSWidgetText<stringLength>::_text=nowStr;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 21: return _selectColor;
            case 22: return (uint8_t) _selectType;
        }
        return MWOSWidgetText<stringLength>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /**
     * Напечатать спецсимвол
     * @param ch
     * @param posX
     * @param posY
     * @param chW
     * @param chH
      * @param printNow Напечатать (или только вернуть ширину)
     * @return  Ширина напечатанного спецсимвола
     */
    virtual uint16_t printSuperChar(char ch, int16_t posX, int16_t posY, uint16_t chW, uint16_t chH, bool printNow) {
        if (!printNow) return 0;
        if (ch==2) {
            if (_selectType==MWOSPinCodeSelectType::arrow) {
                uint16_t rSize=chW-3;
                int16_t rX= posX+2;
                MWOSWidgetText<stringLength>::displayModule->fillTriangleArrow(rX,posY-rSize-2,rSize,rSize,UpCenter,_selectColor);
                MWOSWidgetText<stringLength>::displayModule->fillTriangleArrow(rX,posY+chH+2,rSize,rSize,DownCenter,_selectColor);
            }
            MWOSWidgetText<stringLength>::displayModule->setTextColor(_selectColor);
            MWOSWidgetText<stringLength>::_colorCount=1; // через 1 символ - вернем цвет
            //MWOSWidgetText<stringLength>::displayModule->drawRect(posX,posY,chW,chH,MWOS_COLOR_GRAY);
        }
        return 0;
    }


    /**
     *
     * @return Код введен полностью
     */
    bool isEnter() {
        return pos>=MWOSWidgetTextValue<stringLength>::_value.length();
    }

    /**
     * Начать ввод нового pin-кода
     */
    void StartNewPinCode() {
        pos=0;
        MWOSWidgetTextValue<stringLength>::_value=MWOSWidgetText<stringLength>::_text;
    }

private:
    int16_t pos=0;
};


#endif //MWOS3_ESP32S3_MWOSWidgetPinCode_H
