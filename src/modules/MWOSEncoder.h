#ifndef MWOS35_MWOSENCODER_H
#define MWOS35_MWOSENCODER_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include "MWOSCounter.h"

#if defined(ESP32) || defined(ESP8266)
#include <FunctionalInterrupt.h>
#endif

/**
* Модуль энкодера (Версия MWOS3.5)
*
* Поддерживает программный режим, прерывания и аппаратные счетчики.
* Измеряет количество импульсов или частоту вращения.
* Режим частотомера: измеряет длительность импульсов для расчета частоты.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра (после сохранения).
*   EVENT_UPDATE - тиковая обработка (чтение пинов, расчет скорости, таймеры).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSEncoder : public MWOSCounter {
public:
    // --- Объявление параметров (автоматическая регистрация) ---

    // считать скорость (подсчет скорости может отрицательно влиять на производительность)
    MWOS_PARAM(10, needSpeed, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // инверсия направления
    MWOS_PARAM(11, invertDirection, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // подтяжка всех портов (0-нет, 1 - на 0, 2 - на питание)
    MWOS_PARAM_FF(12, pull, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1, "Выход;Открытый коллектор;ШИМ;ЦАП");
    // инверсия уровней портов
    MWOS_PARAM(13, invertSignal, PARAM_BITS1, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // порт A
    MWOS_PARAM(14, pinA, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);
    // порт B
    MWOS_PARAM(15, pinB, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);
    // порт референции
    MWOS_PARAM(16, pinRef, MWOS_PIN_INT_PTYPE, PARAM_TYPE_PIN, MWOS_STORAGE_FOR_PIN, 1);
    // длина минимального сигнала [mkS], ниже которого считаем, что это помеха
    MWOS_PARAM(17, filter, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // [сек/1000] через какое время выдавать значения энкодера на сервер
    MWOS_PARAM(18, sendTime, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // [mS] минимальная длина импульса энкодера для подсчета скорости
    MWOS_PARAM(19, stopTime, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // частота (только чтение)
    MWOS_PARAM(20, speed, PARAM_FLOAT32, PARAM_TYPE_READONLY, MWOS_STORAGE_RTC, 1);

    // --- Локальные переменные ---

    // время остановки энкодера в микросекундах
    uint32_t _stopTimeMKS = 0;
    // фильтр минимальной длительности сигнала
    int16_t _filter = 0;
    // время отправки на сервер
    int16_t _sendTime = 200;
    // время остановки (таймаут для скорости)
    int32_t _stopTime = 60;
    // флаг необходимости расчета скорости
    uint8_t _needSpeed : 1;
    // флаг инверсии направления
    uint8_t _invertDirection : 1;
    // тип подтяжки портов (0-3): Выход;Открытый коллектор;ШИМ;ЦАП
    uint8_t _pull : 3;
    // флаг инверсии сигналов
    uint8_t _invertSignal : 1;
    // флаг инициализации программного режима
    uint8_t initedSoft = false;
    // последнее состояние порта A
    uint8_t lastPortA = false;
    // номер порта A
    MWOS_PIN_INT _pinA = -1;
    // номер порта B
    MWOS_PIN_INT _pinB = -1;
    // номер порта референции
    MWOS_PIN_INT _pinRef = -1;

#ifdef STM32_MCU_SERIES
    PinName _pinBName;
#endif

    MWOSEncoder() : MWOSCounter(CT_HARDWARE, (char *) F("encoder")) {
        moduleType = MODULE_ENCODER;
        _pull = 0;
        _needSpeed = 0;
        _invertDirection = 0;
        _invertSignal = 0;
    }

    MWOSEncoder(MWOS_PIN_INT pinA, MWOS_PIN_INT pinB, MWOS_PIN_INT pinRef = -1) : MWOSEncoder() {
        _pinA = pinA;
        _pinB = pinB;
        _pinRef = pinRef;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        // Сначала обрабатываем события базового класса (счетчика)
        MWOSCounter::onEvent(modeEvent, data);

        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                _pull = loadValueInt(_pull, p_pull, 0);
                _pinA = loadValueInt(-1, p_pinA, 0);
                _pinB = loadValueInt(-1, p_pinB, 0);
                _pinRef = loadValueInt(-1, p_pinRef, 0);
                _filter = loadValueInt(_filter, p_filter, 0);
                _sendTime = loadValueInt(_sendTime, p_sendTime, 0);
                _stopTime = loadValueInt(_stopTime, p_stopTime, 0);
                _stopTimeMKS = (uint32_t)_stopTime * 1000UL;
                _needSpeed = loadValueInt(_needSpeed, p_needSpeed, 0);
                _invertDirection = loadValueInt(_invertDirection, p_invertDirection, 0);
                _invertSignal = loadValueInt(_invertSignal, p_invertSignal, 0);

                initedSoft = false;
                if (_pinA >= 0 && _pinB >= 0) {
                    initedSoft = (_type == CT_SOFTWARE);

                    if (_type == CT_INTERRUPT) {
                        MWOSPins * pinObj = mwos.getPin(_pinA);
                        if (pinObj != NULL) {
                            pinObj->detach();
#if (defined(ESP32) || defined(ESP8266))
                            int iMode = RISING;
                            if ((_invertSignal ^ _invertDirection)) iMode = FALLING;
                            pinObj->attach(std::bind(&MWOSEncoder::encodeStep, this), iMode);
#endif
                        }
                    }
                }
#ifdef STM32_MCU_SERIES
                if (_pinB >= 0) _pinBName = digitalPinToPinName(_pinB);
#endif
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_needSpeed:
                        _needSpeed = data.toInt();
                        SetParamChanged(&p_needSpeed);
                        break;
                    case id_invertDirection:
                        _invertDirection = data.toInt();
                        SetParamChanged(&p_invertDirection);
                        break;
                    case id_pull:
                        _pull = data.toInt();
                        SetParamChanged(&p_pull);
                        break;
                    case id_invertSignal:
                        _invertSignal = data.toInt();
                        SetParamChanged(&p_invertSignal);
                        break;
                    case id_pinA:
                        _pinA = data.toInt();
                        SetParamChanged(&p_pinA);
                        break;
                    case id_pinB:
                        _pinB = data.toInt();
                        SetParamChanged(&p_pinB);
                        break;
                    case id_pinRef:
                        _pinRef = data.toInt();
                        SetParamChanged(&p_pinRef);
                        break;
                    case id_filter:
                        _filter = data.toInt();
                        SetParamChanged(&p_filter);
                        break;
                    case id_sendTime:
                        _sendTime = data.toInt();
                        SetParamChanged(&p_sendTime);
                        break;
                    case id_stopTime:
                        _stopTime = data.toInt();
                        _stopTimeMKS = (uint32_t)_stopTime * 1000UL;
                        SetParamChanged(&p_stopTime);
                        break;
                    case id_speed:
                        // Команда перезапуска замера скорости
                        if (data.toInt() == 0) {
                            timeoutStep = 0;
                            timeStep = micros();
                        }
                        SetParamChanged(&p_speed);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                if (initedSoft) {
                    bool nowPortA = digitalRead(_pinA);
                    if (lastPortA != nowPortA) {
                        if ((_invertSignal ^ _invertDirection) && (!nowPortA)) encodeStep();
                        else if (nowPortA) encodeStep();
                        lastPortA = nowPortA;
                    }
                }
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_needSpeed:
                        data.setValueInt(_needSpeed);
                        break;
                    case id_invertDirection:
                        data.setValueInt(_invertDirection);
                        break;
                    case id_pull:
                        data.setValueInt(_pull);
                        break;
                    case id_invertSignal:
                        data.setValueInt(_invertSignal);
                        break;
                    case id_pinA:
                        data.setValueInt(_pinA);
                        break;
                    case id_pinB:
                        data.setValueInt(_pinB);
                        break;
                    case id_pinRef:
                        data.setValueInt(_pinRef);
                        break;
                    case id_filter:
                        data.setValueInt(_filter);
                        break;
                    case id_sendTime:
                        data.setValueInt(_sendTime);
                        break;
                    case id_stopTime:
                        data.setValueInt(_stopTime);
                        break;
                    case id_speed:
                        data.setValueDouble(getSpeed());
                        break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }
    }

    /**
    * Возвращает текущую частоту вращения энкодера
    * Если не было импульсов более stopTime - считает это остановкой энкодера (скорость 0)
    * @return Частота в Гц
    */
    double getSpeed() {
        if (!_needSpeed || timeoutStep == 0 || _direction == ED_STOP) return 0;

        if (_stopTime > 0) {
            uint32_t nowTime = micros();
            uint32_t diff;
            if (nowTime >= timeStep) {
                diff = nowTime - timeStep;
            } else {
                diff = UINT32_MAX - timeStep + nowTime;
            }

            if (diff > _stopTimeMKS) {
                _direction = ED_STOP;
                return 0;
            }
        }
        return ((double)1000000) / ((double)timeoutStep);
    }

    /**
    * Обработчик шага энкодера (вызывается по прерыванию или из программы)
    */
    void encodeStep() {
#ifdef STM32_MCU_SERIES
        bool nowDirection = digitalReadFast(_pinBName);
#else
        bool nowDirection = digitalRead(_pinB);
#endif

        if (nowDirection ^ _invertDirection) _counterPos++;
        else _counterPos--;

        // Расчет времени импульса для скорости
        if (_needSpeed) {
            if (_direction == nowDirection) {
                uint32_t nowTime = micros();
                uint32_t diff;
                if (nowTime >= timeStep) {
                    diff = nowTime - timeStep;
                } else {
                    diff = UINT32_MAX - timeStep + nowTime;
                }
                timeoutStep = diff;
                timeStep = nowTime;
            } else {
                _direction = (MWOSEncoderDirection)nowDirection;
                timeoutStep = 0;
                timeStep = micros();
            }
        }
    }

protected:
    // Переопределение методов базового класса для инициализации портов при EVENT_CHANGE
    void initPins() {
        if (_pinA >= 0) {
            if (!mwos.mode(_pinA, (MW_PIN_MODE)(_pull + MW_PIN_OUTPUT))) {
                _pinA = -3;
                saveValueInt(-3, p_pinA, 0);
                SetParamChanged(&p_pinA);
            }
        }
        if (_pinB >= 0) {
            if (!mwos.mode(_pinB, (MW_PIN_MODE)(_pull + MW_PIN_OUTPUT))) {
                _pinB = -3;
                saveValueInt(-3, p_pinB, 0);
                SetParamChanged(&p_pinB);
            }
        }
        if (_pinRef >= 0) {
            if (!mwos.mode(_pinRef, (MW_PIN_MODE)(_pull + MW_PIN_OUTPUT))) {
                _pinRef = -3;
                saveValueInt(-3, p_pinRef, 0);
                SetParamChanged(&p_pinRef);
            }
        }
    }
};

#endif
