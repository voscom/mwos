#ifndef MWOS35_MWOSCMD_H
#define MWOS35_MWOSCMD_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"

/**
* Модуль простого соединения через поток (Версия MWOS3.5)
*
* Принимает простые текстовые команды вида:
*   {модуль:параметр:индекс="значения,через,запятую"}
* или запрос:
*   {модуль:параметр:индекс=?}
* отвечает новым значением модуля:
*   ({модуль:параметр:индекс="значения,через,запятую"})
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_UPDATE - чтение данных из потока.
*   EVENT_SET_VALUE - обработка команд установки значений.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSCmd : public MWOSModule {
public:
    // --- Локальные переменные ---

    enum OxyConnectStep: uint8_t {
        NOW_WAIT_START,
        NOW_WAIT_MODULE,
        NOW_WAIT_PARAM,
        NOW_WAIT_INDEX,
        NOW_WAIT_VALUE,
    };

    // текущий шаг парсинга
    OxyConnectStep _step = NOW_WAIT_START;
    // статус соединения
    uint8_t _status;
    // символ кавычек
    char _kavychki = 0;
    // автоматически считывать из стрима данные
    bool autoReadStream = true;
    // указатель на поток
    Stream * _stream;
    // имя модуля из команды
    String _moduleName = "";
    // имя параметра из команды
    String _paramName = "";
    // индекс параметра из команды
    String _paramIndex = "";
    // значение из команды
    String _valueStr = "";
    // ответ на команду
    String _responseStr = "";
    // таймаут чтения
    MWTimeout<uint16_t> _readTimeout;

    // --- Объявление параметров (автоматическая регистрация) ---

    // статус соединения (0-не на связи)
    MWOS_PARAM(0, status, PARAM_UINT8, PARAM_TYPE_CONTROL, MWOS_STORAGE_NO, 1);

    MWOSCmd() : MWOSModule((char *) F("cmd")) {
        moduleType = MODULE_UNDEFINED;
    }

    MWOSCmd(Stream * stream) : MWOSCmd() {
        _stream = stream;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация
            case EVENT_INIT: {
                // Инициализация при старте
            } break;

            // 2. Тиковая обработка (чтение из потока)
            case EVENT_UPDATE: {
                if (autoReadStream && _stream != NULL) {
                    while (_stream->available() > 0) {
                        char ch = _stream->read();
                        #if MWOS_LOG_CMD > 4
                        MW_LOG('~'); MW_LOG(ch);
                        #endif
                        readNextByte(ch);
                        responseSend();
                    }
                }
            } break;

            // 3. Установка значения (замена onReceiveValue)
            case EVENT_SET_VALUE: {
                if (data.param_id == id_status) {
                    // Обработка установки статуса
                    _status = data.toInt();
                    MWOSModuleBase::SetParamChanged(&p_status);
                }
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_status:
                        data.setValueInt(_status);
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSModule::onEvent(modeEvent, data);
    }

    /**
    * Отправить подготовленный ответ
    */
    void responseSend() {
        if (_responseStr.length() > 0) {
            _stream->print(_responseStr);
            _responseStr = "";
        }
        if (_status == 1 && _readTimeout.isTimeout(3000)) {
            _status = 0;
            MWOSModuleBase::SetParamChanged(&p_status);
        }
    }

    /**
    * Обрабатывает очередной входящий байт в поисках своего пакета
    * @param ch Очередной входящий байт
    */
    void readNextByte(char ch) {
        switch (_step) {
            case NOW_WAIT_START: {
                if (ch == '{') SetStep(NOW_WAIT_MODULE);
            } break;
            case NOW_WAIT_MODULE: {
                if (ch == ':') SetStep(NOW_WAIT_PARAM);
                else if (ch == '{') SetStep(NOW_WAIT_MODULE);
                else _moduleName += ch;
            } break;
            case NOW_WAIT_PARAM: {
                if (ch == ':') SetStep(NOW_WAIT_INDEX);
                else if (ch == '{') SetStep(NOW_WAIT_MODULE);
                else if (ch == '=') SetStep(NOW_WAIT_VALUE);
                else _paramName += ch;
            } break;
            case NOW_WAIT_INDEX: {
                if (ch == '=') SetStep(NOW_WAIT_VALUE);
                else if (ch == '{') SetStep(NOW_WAIT_MODULE);
                else _paramIndex += ch;
            } break;
            case NOW_WAIT_VALUE: {
                if ((ch == '"' || ch == '\'' || ch == '~' || ch == '#' || ch == '^') && _valueStr == "") _kavychki = ch;
                else if (ch == _kavychki && ch != 0) _kavychki = 0;
                else if (ch == '}' && _kavychki == 0) onReciveCommand();
                else _valueStr += ch;
            } break;
        }
        if (_step != NOW_WAIT_START && _readTimeout.isTimeout(3000)) {
            SetError(99);
        }
    }

    /**
    * Установить новый шаг парсинга
    * @param newStep Новый шаг
    */
    void SetStep(OxyConnectStep newStep) {
        _step = newStep;
        switch (_step) {
            case NOW_WAIT_START: {
                _kavychki = 0;
                _moduleName = "";
                _paramName = "";
                _valueStr = "";
                _paramIndex = "";
                _readTimeout.stop();
            } break;
            case NOW_WAIT_MODULE: {
                _moduleName = "";
                _readTimeout.start();
            } break;
            case NOW_WAIT_PARAM: {
                _paramName = "";
            } break;
            case NOW_WAIT_INDEX: {
                _paramIndex = "";
            } break;
            case NOW_WAIT_VALUE: {
                _kavychki = 0;
                _valueStr = "";
            } break;
        }
    }

    /**
    * Установить ошибку
    * @param errorCode Код ошибки
    * @return Код ошибки
    */
    uint8_t SetError(int8_t errorCode) {
        SetStep(NOW_WAIT_START);
        if (errorCode != 0) {
            #if MWOS_LOG_CMD > 0
            MW_LOG_MODULE(this); MW_LOG(F("Error: ")); MW_LOG_LN(errorCode);
            #endif
        }
        return errorCode;
    }

    /**
    * Получена команда в формате:
    * {модуль:параметр:индекс="значения,через,запятую"}
    * _moduleName=модуль
    * _paramName=параметр
    * _paramIndex=индекс
    * _valueStr=значения,через,запятую (может быть в кавычках " или ')
    * @return Код ошибки (0=успех)
    */
    uint8_t onReciveCommand() {
        _paramName.trim();
        _moduleName.trim();
        _paramIndex.trim();

        if (_moduleName.isEmpty()) return SetError(11);
        if (_paramName.isEmpty()) _paramName = "*";

        String res = "";
        String address = _moduleName + ':' + _paramName;
        if (!_paramIndex.isEmpty()) address += ':' + _paramIndex;

        if (address == "controller:name") res = String(mwos.name);
        else if (address == "controller:id") res = String(mwos.getCID());
        else if (address == "controller:chip") res = String(getChipID());
        else if (address == "controller:modules") {
            MWOSModuleBase * moduleNext = (MWOSModuleBase *) mwos.child;
            while (moduleNext != NULL && moduleNext->unitType == UnitType::MODULE) {
                if (!res.isEmpty()) res += ',';
                res += String(moduleNext->name);
                moduleNext = (MWOSModuleBase *) moduleNext->next;
            }
        } else {
            MWOSModule * moduleObj = (MWOSModule *) mwos.FindChildByName(_moduleName.c_str());
            if (moduleObj == NULL) return SetError(12);

            if (_paramName == "*") {
                if (_valueStr == "?") {
                    MWOSParam * paramNext = (MWOSParam *) moduleObj->child;
                    while (paramNext != NULL && paramNext->unitType == UnitType::PARAM) {
                        if (!res.isEmpty()) res += ',';
                        res += String(paramNext->name);
                        res += ':';
                        res += String(paramNext->arrayCount());
                        paramNext = (MWOSParam *) paramNext->next;
                    }
                }
            } else {
                MWOSParam * paramObj = (MWOSParam *) moduleObj->FindChildByName(_paramName.c_str());
                if (paramObj == NULL) return SetError(13);

                if (!_valueStr.isEmpty() && _valueStr != "?") {
                    moduleObj->setValueStrToParam(_valueStr, paramObj, _paramIndex, true);
                }
                // отправим ответ
                res = moduleObj->getValueStrFromParam(paramObj, _paramIndex);
            }
        }

        address = "{" + address + "=" + res + "}";
        #if MWOS_LOG_CMD > 2
        MW_LOG_MODULE(this); MW_LOG_LN(address);
        #endif

        _paramName = "";
        _moduleName = "";
        _paramIndex = "";
        _responseStr = '(' + address + ')';

        if (_status == 0) {
            _status = 1;
            MWOSModuleBase::SetParamChanged(&p_status);
        }

        return SetError(0);
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif