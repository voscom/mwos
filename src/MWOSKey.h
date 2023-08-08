#ifndef MWOS3_MWOSKEY_H
#define MWOS3_MWOSKEY_H

#include "core/adlib/MWBitsMaskStat.h"

/***
 * Несколько однотипных ключей (можно один)
 * Например: Это могут быть несколько реле на одном блоке
 */
template<MWOS_PARAM_INDEX_UINT keysCount>
class MWOSKey : public MWOSModule {
public:

#pragma pack(push,1)
    // состояния ключей (0-вкл, 1-выкл)
    //uint8_t _value[keysCount];
    MWBitsMaskStat<keysCount> _turn;
    // порты
    MWOS_PIN_INT _pin[keysCount];
    // инвертировать значение
    uint8_t _invert:1;
    // всегда производить инициализацию ключа перед переключением
    uint8_t _reinit:1;
    // всегда отключено (одновременно c _invert - всегда включено)
    uint8_t _alwaysoff:1;
    // подтяжка (0-нет, 1-на 0, 2-на питание, 3 - открытый коллектор)
    uint8_t _pull:2;
    // влияние на другие ключи этого модуля: 0-нет, 1-выключить все другие при включении этого
    uint8_t _checkbox:1;
    // резерв для выравнивания по байту
    uint8_t _reserve:2;
#pragma pack(pop)

    // состояния ключей (0-вкл, 1-выкл)
    MWOS_PARAM(0, turn, mwos_param_bits1, mwos_param_control, mwos_param_storage_rtc, keysCount);
    // порты
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, mwos_param_storage_eeprom, keysCount);
    // инвертировать значение
    MWOS_PARAM(2, invert, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // всегда производить инициализацию ключа перед переключением
    MWOS_PARAM(3, reinit, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    //  всегда отключено (одновременно c _invert - всегда включено)
    MWOS_PARAM(4, alwaysoff, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    //подтяжка (0-нет, 1-на 0, 2-на питание, 3 - открытый коллектор)
    MWOS_PARAM(5, pull, mwos_param_bits2, mwos_param_option, mwos_param_storage_eeprom, 1);
    // влияние на другие ключи этого модуля: 0-нет, 1-выключить все другие при включении этого
    MWOS_PARAM(6, checkbox, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);


    MWOSKey() : MWOSModule((char *) F("key")) {
        AddParam(&p_turn);
        AddParam(&p_pin);
        AddParam(&p_invert);
        AddParam(&p_reinit);
        AddParam(&p_alwaysoff);
        AddParam(&p_pull);
        AddParam(&p_checkbox);
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            _pin[i]=-1;
        }
    }

    /***
     * Создать ключи
     * @param ports ссылка на область PROGMEM с номерами портов по умолчанию (по 8 бит на порт)
     */
    MWOSKey(const uint8_t * pins) : MWOSKey() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < keysCount; ++i) {
            _pin[i]=pgm_read_byte_near(pins + i);
        }
    }

    MWOSKey(MWOS_PIN_INT pin) : MWOSKey() {
        _pin[0]=pin;
    }

    virtual void onInit() {
        _invert=loadValue(_invert, &p_invert);
        _reinit=loadValue(_reinit, &p_reinit);
        _alwaysoff=loadValue(_alwaysoff, &p_alwaysoff);
        _pull=loadValue(_pull, &p_pull);
        _checkbox=loadValue(_checkbox, &p_checkbox);
        for (MWOS_PARAM_INDEX_UINT index = 0; index < keysCount; ++index) {
            _pin[index]=loadValue(_pin[index], &p_pin, index);
            _turn.setBit(loadValue(_turn.getBit(index), &p_turn, index),index);
            initKey(index);
            turn(loadValue(_invert, &p_turn, index), index, false);
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex= 0) {
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 0: return getValueBool(arrayIndex);
            case 1: return _pin[arrayIndex];
            case 2: return _invert;
            case 3: return _reinit;
            case 4: return _alwaysoff;
            case 5: return _pull;
            case 6: return _checkbox;
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual void setValue(int64_t v, MWOSParam * param, int16_t arrayIndex= 0) {
        MW_LOG_MODULE(this,arrayIndex); MW_LOG(F("setValue: ")); MW_LOG_LN((int32_t) v);
        if (param==&p_turn) { // команда установки ключа
            turn(v,arrayIndex, true);
        } else {
            MWOSModule::setValue(v,param,arrayIndex);  // сохраним в хранилище
            onInit(); // обновим настройки
        }
    }

    void initKey(int16_t index) {
        mwos.pin(_pin[index])->mode(true, _pull);
        MW_LOG_MODULE(this,index); MW_LOG(F("init key ")); MW_LOG(_pin[index]); MW_LOG('='); MW_LOG_LN(_pull);
    }

    void turnBool(bool modeTurn, int16_t index=0) {
        if (modeTurn==getValueBool(index)) return;
        turn(modeTurn,index, true);
    }

    bool getValueBool(int16_t index=0) {
        return _turn.getBit(index);
    }

    /**
     * Изменить значение ключа. (В отличие от turnBool не кеширует значение)
     * @param mode 0-выключить,1-включить,2-переключить, 3-255-оставить как есть
     */
    virtual void turn(uint8_t mode, int16_t index=0, bool saveToStorage=true) {
        MW_LOG_MODULE(this,index); MW_LOG(F("turn ")); MW_LOG_LN(mode);
        uint8_t v=_turn.getBit(index);
        uint8_t old_v=v;
        if (_alwaysoff) {
            v=0;
        } else {
            if (mode<2) v=mode;
            else if (mode==2) {
                if (v==0) v=1;
                else v=0;
            }
        }
        if (v != old_v)  {
            _turn.setBit(v,index);
            MW_LOG_MODULE(this,index); MW_LOG('='); MW_LOG_LN(_turn.getBit(index));
            SetParamChanged(&p_turn, index, true); // отправить изменение ключа в стандартном порядке
            if (_checkbox && v==1) { // отключим другие ключи, если включен режим checkbox
                for (MWOS_PARAM_INDEX_UINT i = 0; i < (int32_t) keysCount; ++i) if (i != index) { // все ключи, кроме этого
                        turnBool(false,i); // отключим ключ
                }
            }
            if (_invert) {
                if (v==1) v=0;
                else v=1;
            }
            if (_reinit) initKey(index);
            if (_pin[index]>=0) mwos.pin(_pin[index])->writeDigital(v);
            if (saveToStorage) saveValue(old_v, &p_turn, index);
        }

    }

};


#endif //MWOS3_MWOSKEY_H
