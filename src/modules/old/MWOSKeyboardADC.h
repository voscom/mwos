#ifndef MWOS3_MWOSKeyboardADC_H
#define MWOS3_MWOSKeyboardADC_H
#include "../../core/MWOSKeyboardBase.h"
#ifdef ESP32
#include <driver/adc.h>
#endif

/***
 * Клавиатура на порту АЦП, где все клавиши подключены через резисторную матрицу
 *
 * @tparam countKeys   Количество клавиш
 */
template<uint8_t countKeys>
class MWOSKeyboardADC : public MWOSKeyboardBase {
public:

#pragma pack(push,1)
    MWOS_PIN_INT _pin=-1;
    uint16_t _values[countKeys+1];
#pragma pack(pop)

    // пин для подключения клавиатуры
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_option, MWOS_STORAGE_FOR_PIN, 1);
    // среднее цифровое значения в порту ADC, соответствующее каждой клавише (последнее значение соответствует не нажатым клавишам)
    MWOS_PARAM(2, values, mwos_param_uint16 , mwos_param_option, MWOS_STORAGE_EEPROM, countKeys+1);

    /**
     * Создать модуль клавиатуры АЦП без настроек
     * все настройки - удаленно
     */
    MWOSKeyboardADC() : MWOSKeyboardBase() {
        AddParam(&p_pin);
        AddParam(&p_values);
        // предварительно заполним значения
        _values[countKeys]=4096; // 12 бит АЦП
        for (uint8_t i = 0; i < countKeys; i++) {
            _values[i] = (4096/countKeys) * i;
        }
    }

    /**
     * Создать модуль клавиатуры АЦП
     * @param pin       пин АЦП
     * @param codes     коды клавиш
     * @param values    среднее цифровое значения в порту ADC, соответствующее каждой клавише
     */
    MWOSKeyboardADC(MWOS_PIN_INT pin, uint8_t * codes=NULL, uint16_t * values=NULL) : MWOSKeyboardADC() {
        _pin=pin;
        _codes=codes;
        // заполним значения
        if (values!=NULL) {
            for (uint8_t i = 0; i < countKeys; i++) {
                _values[i]= pgm_read_word_near(values + i*2);
                _values[i] = (4096/countKeys) * i;
            }
        }
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSKeyboardBase::onEvent(modeEvent);
        if (modeEvent==EVENT_INIT || modeEvent==EVENT_CHANGE) { // инициализация или изменение параметров
            _pin = loadValue(_pin, &p_pin, 0);
            for (uint8_t index = 0; index <= countKeys; index++) {
                _values[index] = loadValue(_values[index], &p_values, index);
            }
            _timeout.startMS(100);
        } else
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            if (_pin<0) return;
            if (!_timeout.isTimeout()) return;
            testPressedKeys();
            _timeout.startMS(100); // что-бы один цикл длился 100 МСек
        }
    }

    /**
     * Проверить - есть ли нажатые клавиши
     */
    void testPressedKeys() {
        int nowValue;
#ifdef CONFIG_IDF_TARGET_ESP32C3
        if (_pin==GPIO_NUM_5) { // только для ножки GPIO_NUM_5 (ADC2) на ESP32C3 - ЦАП может возвращать ошибку при включенном WiFi
            if (adc2_get_raw(ADC2_CHANNEL_0,ADC_WIDTH_BIT_12, &nowValue)!=ESP_OK) return; // для esp32c3 adc2 проверяем успешно ли считывание
        } else
#endif
        nowValue=mwos.readValueAnalog(_pin);
        for (uint8_t i = 0; i < countKeys; i++) {
            if ((_values[i]+_values[i+1])/2 > nowValue) {
                pressKey(i);
                return;
            }
        }
        _timeoutPress.start(0); // отпустили нажатую кнопку
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 1: return _pin;
            case 2: return _values[arrayIndex];
        }
        return MWOSKeyboardBase::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

};


#endif //MWOS3_MWOSKeyboardADC_H
