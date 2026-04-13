#ifndef MWOS35_MWOSKEYBASE_H
#define MWOS35_MWOSKEYBASE_H

#include "core/MWOSModule.h"
#include "core/adlib/MWBitsMaskStat.h"

#ifndef MWOS_PARAM_STORAGE_KEY_TURN
// хранилище, в котором сохранять состояния ключей
#define MWOS_PARAM_STORAGE_KEY_TURN MWOS_STORAGE_RTC
#endif

// режимы работы ключей при старте
enum KeyStartMode {
    // восстанавливать состояние до выключения из хранилища
    KSM_RESTORE = 0,
    // не трогать (задать вручную)
    KSM_MANUAL = 1,
    // выключено
    KSM_OFF = 2,
    // включено
    KSM_ON = 3,
};

/**
* Базовый класс на несколько однотипных устройств, типа ключа
* можно наследовать для любых устройств, требующих включения/выключения
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_GET_VALUE - возврат значений через switch.
*   EVENT_SET_VALUE - обработка команд установки значений.
*/
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKeyBase : public MWOSModule {
public:
    // --- Объявление параметров (автоматическая регистрация) ---

    // состояния ключей (0=выкл, 1=вкл)
    MWOS_PARAM(0, turn, PARAM_BITS1, PARAM_TYPE_CONTROL, MWOS_PARAM_STORAGE_KEY_TURN, keysCount);
    // инвертировать значение
    MWOS_PARAM(1, invert, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // всегда отключено (одновременно с invert - всегда включено)
    MWOS_PARAM(2, alwaysoff, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // влияние на другие ключи этого модуля: 0=нет, 1=выключить все другие при включении этого
    MWOS_PARAM(3, checkbox, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // Значение turn при включении микроконтроллера
    MWOS_PARAM_FF(4, startTurn, PARAM_BITS2, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1,
                  "Восстановить;Не трогать;Выключено;Включено");

    // --- Локальные переменные ---

    // состояния ключей (0=выкл, 1=вкл)
    MWBitsMaskStat<keysCount> _turn;
    // инвертировать значение
    uint8_t _invert : 1;
    // всегда отключено
    uint8_t _alwaysoff : 1;
    // влияние на другие ключи: 0=нет, 1=выключить другие при включении этого
    uint8_t _checkbox : 1;
    // Значение turn при включении микроконтроллера
    uint8_t _startTurn : 2;
    // подтяжка 'Выход;Открытый коллектор;ШИМ;ЦАП
    uint8_t _pull : 3;

    MWOSKeyBase() : MWOSModule((char *) F("key")) {
        moduleType = MODULE_KEY;
    }

    /**
     * Задать тип порта на выход
     * @param pinMode
     */
    void mode(MW_PIN_MODE pinOutputMode) {
        _pull=pinOutputMode & 7;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                _invert = MWOSModule::loadValueInt(_invert, p_invert, 0);
                _alwaysoff = MWOSModule::loadValueInt(_alwaysoff, p_alwaysoff, 0);
                _checkbox = MWOSModule::loadValueInt(_checkbox, p_checkbox, 0);
                _startTurn = MWOSModule::loadValueInt(_startTurn, p_startTurn, 0);

                for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
                    bool nowTurn = _turn.getBit(index);
                    if (_startTurn == KSM_RESTORE) {
                        nowTurn = MWOSModule::loadValueInt(nowTurn, p_turn, index);
                    }
                    if (_startTurn > KSM_MANUAL) {
                        nowTurn = _startTurn - 2;
                    }
                    _turn.setBit(!nowTurn, index); // чтобы точно переключилось - поменяем состояние
                    turn(nowTurn, index, false);
                }
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_turn:
                        turn(data.toInt(), data.param_index, true);
                        break;
                    case id_invert:
                        _invert = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_invert);
                        break;
                    case id_alwaysoff:
                        _alwaysoff = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_alwaysoff);
                        break;
                    case id_checkbox:
                        _checkbox = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_checkbox);
                        break;
                    case id_startTurn:
                        _startTurn = data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_startTurn);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_turn:
                        data.setValueInt(getValueBool(data.param_index));
                        return;
                    case id_invert:
                        data.setValueInt(_invert);
                        return;
                    case id_alwaysoff:
                        data.setValueInt(_alwaysoff);
                        return;
                    case id_checkbox:
                        data.setValueInt(_checkbox);
                        return;
                    case id_startTurn:
                        data.setValueInt(_startTurn);
                        return;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSModule::onEvent(modeEvent, data);
    }

    /**
    * Задать тип ключа
    * @param pull Настройка пина ключа MW_PIN_EVENT_OUTPUT*
    */
    void setMode(MW_PIN_MODE pull) {
        if (pull < MW_PIN_OUTPUT || pull > MW_PIN_OUTPUT_DAC) {
            pull = MW_PIN_OUTPUT;
        }
        _pull = pull - MW_PIN_OUTPUT;
    }

    /**
    * Переключить ключ только если состояние отличается от текущего
    */
    void turnBool(bool modeTurn, int16_t index) {
        if (modeTurn == getValueBool(index)) return;
        turn(modeTurn, index, true);
    }

    /**
    * Получить текущее бинарное значение ключа
    */
    bool getValueBool(int16_t index) {
        return _turn.getBit(index);
    }

    /**
    * Выключить ключ
    */
    void stop(int16_t index) {
        turn(0, index, true);
    }

    /**
    * Включить ключ
    */
    void start(int16_t index) {
        turn(1, index, true);
    }

    /**
    * Переключить ключ в противоположное состояние
    */
    void restart(int16_t index) {
        turn(2, index, true);
    }

    /**
    * Изменить значение ключа
    * @param mode 0=выключить, 1=включить, 2=переключить, 3-255=оставить как есть
    * @param index Индекс ключа
    * @param saveToStorage Сохранять в хранилище
    * @return Возвращает состояние порта
    */
    virtual uint8_t turn(uint8_t mode, int16_t index, bool saveToStorage) {
        uint8_t v = _turn.getBit(index);
        uint8_t old_v = v;

        if (_alwaysoff) {
            v = 0;
        } else {
            if (mode < 2) {
                v = mode;
            } else if (mode == 2) {
                if (v == 0) v = 1;
                else v = 0;
            }
        }

        if (v != old_v) {
            _turn.setBit(v, index);
            MWOSModuleBase::SetParamChanged(&p_turn, index, true);

            if (_checkbox && v == 1) {
                // отключим другие ключи, если включен режим checkbox
                for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
                    if (i != index) {
                        turnBool(false, i);
                    }
                }
            }

            old_v = v; // состояние ключа без инверсии

            if (saveToStorage && _startTurn == KSM_RESTORE) {
                MWOSModule::saveValueInt(old_v, p_turn, index);
            }
        }

        v ^= _invert; // учтем инверсию
        return v;
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif
