#ifndef MWOS3_ESP32S3_MWOSWIDGETTEXT_H
#define MWOS3_ESP32S3_MWOSWIDGETTEXT_H
/**
 * Виджет дисплея MWOS
 *
 * Показывает текст, заданный в парамере 10 на дисплей
 *
 */
#include <Adafruit_GFX.h>
#include "MWOSWidget.h"

class MWOSWidgetText: public MWOSWidget {
public:

#pragma pack(push,1)
    String _text="";
#pragma pack(pop)

    // тест для показа на дисплее (до 255 символов)
    MWOS_PARAM(10, text, mwos_param_string, mwos_param_option, mwos_param_storage_no, 255);

    MWOSWidgetText(MWOSDisplay * _displayModule) : MWOSWidget(_displayModule,(char *) F("widgetText")) {
        AddParam(&p_text);
    }

/*    virtual void onInit() {
        MWOSWidget::onInit();
    }
*/
    /***
     * Получена команда для параметра. Вызывается автоматически при получении новой команды или значения по модулю связи
     * Для блока данных без буфферизации - вызывает сначала команду mwos_server_cmd_param_start_block, потом для каждого байта блока,
     * и в конце mwos_server_cmd_param_stop_block или mwos_server_cmd_param_error_block
     * @param reciverDat   Полученные данные со структурой
     * @return Если команда обработана нормально
     */
    virtual bool onReciveCmd(MWOSProtocolCommand cmd, MWOSNetReciverFields * reciverDat) {
        if (reciverDat->module_id==10) {
            switch (cmd) {
#if MWOS_RECIVE_BLOCK_SIZE<0xffff // задан буффер приема
                case mwos_server_cmd_param_set_value:
                    print(reciverDat->GetString());
                    break;
#else   // не задан буффер приема (мало ОЗУ)
                    case mwos_server_cmd_param_start_block:  // начали передачу текста
           case mwos_server_cmd_param_error_block:  // ошибка передачи текста
                _text="";
                break;
           case mwos_server_cmd_param_stop_block: // закончили передачу текста
                print(_text);
                break;
           case mwos_server_cmd_param_set_value:
                _text+=(char) reciverDat->buffer[0]; // очередной байт текста
                break;
#endif
            }
        }
        return MWOSWidget::onReciveCmd(cmd, reciverDat);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==10) return packStringToValue(_text);
        return MWOSWidget::getValue(param, arrayIndex);
    }

    void setText(String text) {
        _text=text;
    }

    /**
     * Обновляет виджет на экране
     */
    virtual void print() {
        displayModule->display->setCursor(_x, _y);
        displayModule->display->setTextSize(_sizeX,_sizeY);
        displayModule->display->setTextColor(_color);
        MW_LOG_MODULE(this); MW_LOG(F("print: ")); MW_LOG_LN(_text);
        displayModule->display->print(_text);
    }

    /**
     * Показать тект на дисплее
     * @param text
     */
    void print(String text) {
        _text=text;
        SetParamChanged(&p_text,0,true);
        displayModule->SetDisplayUpdate();
    }


};


#endif //MWOS3_ESP32S3_MWOSWIDGETTEXT_H
