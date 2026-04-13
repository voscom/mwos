#ifndef MWOS3_MWOSExample_H
#define MWOS3_MWOSExample_H

#include "core/MWOSModule.h"

/**
 * Пример создания нового модуля
 *
 */
class MWOSExample : public MWOSModule {
public:

    String _text="Hell0 world!";

    // статус модуля
    MWOS_PARAM(0, status, PARAM_UINT8, PARAM_TYPE_CONTROL, MWOS_STORAGE_NO, 1);
    // текущее состояние модуля (вкл/выкл)
    MWOS_PARAM(1, turn, PARAM_BITS1, PARAM_TYPE_CONTROL, MWOS_STORAGE_RTC, 3);
    // порт модуля
    MWOS_PARAM(2, pin, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 3);
    // режим порта модуля
    MWOS_PARAM_FF(3, pinMode, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 3, "вход;выход;открытый коллектор");
    // описание модуля (до 19 букв)
    MWOS_PARAM(4, text, PARAM_STRING, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NVS, 1024);
    // коэффициент (вещественный тип)
    MWOS_PARAM(5, coef, PARAM_FLOAT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 3);
    // при старте состояние модуля (вкл/выкл)
    MWOS_PARAM(6, start, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 3);
    // счетчик
    MWOS_PARAM(7, count, PARAM_INT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 3);

    /**
     * Создать модуль. Для каждого модуля необходим основной конструктор без аргументов!
     * конструкторы с аргументами - можно задавать дополнительно.
     * тут необходимо добавить все созданные параметры.
     * и можно переопределить имя и тип модуля.
     */
    MWOSExample() : MWOSModule((char *) F("example")) {
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
        switch (modeEvent) {
            case EVENT_UPDATE: { // каждый такт ОС
            } break;
            case EVENT_INIT: { // только при инициализации ОС
                _text=loadValueString(_text,p_text);

            } break;
            case EVENT_CHANGE: { // при изменении параметра
                MWOSParam * param=getParam(data.param_id);
                MW_LOG_MODULE(this,param,data.param_index); MW_LOG(F("EVENT_CHANGE ")); MW_LOG_LN(data.toInfo());
                switch (data.param_id) {
                    case id_status: {
                    } break;
                    case id_turn: {
                    } break;
                    case id_pin: {
                    } break;
                    case id_pinMode: {
                    } break;
                    case id_text: {
                        _text=data.toString();
                    } break;
                    case id_coef: {
                    } break;
                    case id_start: {
                    } break;
                    case id_count: {
                    } break;
                }
            } break;
            case EVENT_GET_VALUE: { // при запросе параметра
                MWOSParam * param=getParam(data.param_id);
                MW_LOG_MODULE(this,param,data.param_index); MW_LOG(F("EVENT_GET_VALUE ")); MW_LOG_LN(data.toInfo());
                switch (data.param_id) {
                    case id_status: {
                    } break;
                    case id_turn: {
                    } break;
                    case id_pin: {
                    } break;
                    case id_pinMode: {
                    } break;
                    case id_text: {
                        data.setString(_text.c_str());
                        MW_LOG_MODULE(this,param,data.param_index); MW_LOG(F("empty EVENT_GET_VALUE "));  MW_LOG_LN(data.toInfo());
                    } break;
                    case id_coef: {
                    } break;
                    case id_start: {
                    } break;
                    case id_count: {
                    } break;
                }
                MW_LOG_MODULE(this,param,data.param_index); MW_LOG(F("after EVENT_GET_VALUE "));  MW_LOG_LN(data.toInfo());
            } break;
            default: ;
        }
        MWOSModule::onEvent(modeEvent,data);
    }


};


#endif
