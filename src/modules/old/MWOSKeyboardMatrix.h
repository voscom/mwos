#ifndef MWOS3_MWOSKeyboardMatrix_H
#define MWOS3_MWOSKeyboardMatrix_H
#include "../../core/MWOSKeyboardBase.h"
/***
 * Матричная клавиатура
 * можно задать любую матрицу
 *
 * если не задавать пины для записи, то кнопки чтения проверяются на заземление
 * (тогда вместо пина записи можно использовать землю)
 *
 */
template<uint8_t readPins, uint8_t writePins=0>
class MWOSKeyboardMatrix : public MWOSKeyboardBase {
public:

#pragma pack(push,1)
    int8_t _error=0;
    MWOS_PIN_INT _pinRead[readPins];
    MWOS_PIN_INT _pinWrite[writePins];
#pragma pack(pop)

    // пины для подключения клавиатуры, из которых происходит чтение
    MWOS_PARAM(1, pinRead, MWOS_PIN_INT_PTYPE, mwos_param_option, MWOS_STORAGE_FOR_PIN, readPins);
    // пины для подключения клавиатуры, в которые происходит запись (если 0, то нет таких)
    MWOS_PARAM(2, pinWrite, MWOS_PIN_INT_PTYPE, mwos_param_option, MWOS_STORAGE_FOR_PIN, writePins);
    // пины для подключения клавиатуры, в которые происходит запись (если 0, то нет таких)
    MWOS_PARAM(3, error, mwos_param_int8, mwos_param_option, MWOS_STORAGE_NO, writePins);

    MWOSKeyboardMatrix() : MWOSKeyboardBase() {
        AddParam(&p_pinRead);
        if (writePins>0) AddParam(&p_pinWrite);
        AddParam(&p_error);
    }

    MWOSKeyboardMatrix(const uint8_t * pins) : MWOSKeyboardMatrix() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < readPins; ++i) {
            _pinRead[i]=pgm_read_byte_near(pins + i);
            if (_pinRead[i]==255) _pinRead[i]=-1;
        }
        if (writePins>0)
        for (MWOS_PARAM_INDEX_UINT i = 0; i < writePins; ++i) {
            _pinWrite[i]=pgm_read_byte_near(pins + i+readPins);
            if (_pinWrite[i]==255) _pinWrite[i]=-1;
        }
    }

    MWOSKeyboardMatrix(const uint8_t * pins, uint8_t * codes) : MWOSKeyboardMatrix(pins) {
        _codes=codes;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSKeyboardBase::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            for (uint8_t index = 0; index < readPins; ++index) {
                _pinRead[index] = loadValue(_pinRead[index], &p_pinRead, index);
                if (_pinRead[index] >= 0) {
                    if (!mwos.mode(_pinRead[index], MW_PIN_EVENT_INPUT_PULLUP)) {
                        MW_LOG_MODULE(this,&p_pinRead,index); MW_LOG(F("error port: ")); MW_LOG_LN(_pinRead[index]);
                        _pinRead[index]=-3;
                        MWOSModule::SetParamChanged(&p_pinRead,index); // сообщим ошибку
                    }
                }
            }
            if (writePins > 0)
                for (uint8_t index = 0; index < writePins; ++index) {
                    _pinWrite[index] = loadValue(_pinWrite[index], &p_pinWrite, index);
                    if (_pinWrite[index] >= 0) {
                        if (!mwos.mode(_pinWrite[index], MW_PIN_EVENT_OUTPUT)) {
                            MW_LOG_MODULE(this,&p_pinRead,index); MW_LOG(F("error port: ")); MW_LOG_LN(_pinRead[index]);
                            _pinWrite[index]=-3;
                            MWOSModule::SetParamChanged(&p_pinWrite,index); // сообщим ошибку
                        } else
                            mwos.writeValueDigital(_pinWrite[index], HIGH);
                    }
                }
            _error= 100; // тестируем после инициализации на ошибки
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (_timeout.isTimeout()) {
                LoadPressedKeys();
                _timeout.startMS(70);
            }
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 1: return _pinRead[arrayIndex];
            case 2: return _pinWrite[arrayIndex];
            case 3: return _error;
        }
        return MWOSKeyboardBase::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    void SetError(int8_t error) {
        if (error==_error) return;
        _error=error;
        SetParamChanged(&p_error);
        MW_LOG_MODULE(this); MW_LOG(F("error code = "));  MW_LOG_LN(_error);
    }

protected:
    MWTimeout _timeout;

    void LoadPressedKeys() {
        if (writePins>0) {
            for (uint8_t wPin = 0; wPin < writePins; wPin++) {
                for (uint8_t p = 0; p < writePins; p++) mwos.writeValueDigital(_pinWrite[p], (p!=wPin));
                delay(5);
                LoadPressedKey(wPin);
            }
        } else
            LoadPressedKey(0);
    }

    void LoadPressedKey(uint8_t wPin) {
        for (uint8_t rPin = 0; rPin < readPins; rPin++) {
            uint8_t keyNum=wPin*readPins+rPin+1;
            if(!mwos.readValueDigital(_pinRead[rPin]))
                pressKey(keyNum);
            else
                unpressKey(keyNum);
        }
    }


};


#endif //MWOS3_MWOSKeyboardMatrix_H
