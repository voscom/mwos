#ifndef MWOS35_MWOSCOUNTER_H
#define MWOS35_MWOSCOUNTER_H

#include <cstdint>
#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"

// имя модуля
const char MWOS_COUNTER_NAME[] PROGMEM = {"counter"};

// направление движения энкодера
enum MWOSEncoderDirection {
    // назад
    ED_BACKWARD = 0,
    // вперед
    ED_FORWARD = 1,
    // остановлен
    ED_STOP = 2,
    // на паузе
    ED_PAUSE = 3,
};

// тип счетчика
enum MWOSCounterType {
    // программный
    CT_SOFTWARE = 0,
    // на прерываниях
    CT_INTERRUPT = 1,
    // аппаратный (только для потомков MWOSEncoder)
    CT_HARDWARE = 2,
    // альтернативный режим работы энкодера: подсчет импульсов на A - плюс, на B - минус (ВНИМАНИЕ!!! пока только для PCNT)
    CT_HARDWARE_ALTER = 3,
    // подсчет времени от старта (для измерения частоты)
    CT_TIMEOUT = 4,
};

/**
* Счетчик энкодера / таймера (Версия MWOS3.5)
*
* Предназначен для отслеживания количества импульсов или измерения частоты.
* Поддерживает программный режим, прерывания и аппаратные счетчики.
* Режим CT_TIMEOUT позволяет измерять длительность импульсов для расчета частоты.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра (после сохранения).
*   EVENT_UPDATE - тиковая обработка (таймеры, проверка цели).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSCounter : public MWOSModule {
public:
    // --- Объявление параметров (автоматическая регистрация) ---

    // текущая позиция счета без учета шага (можно записывать)
    MWOS_PARAM(0, counter, PARAM_INT32, PARAM_TYPE_CONTROL, MWOS_STORAGE_RTC, 1);
    // текущая позиция c учетом шага (можно записывать)
    MWOS_PARAM(1, position, PARAM_DOUBLE64, PARAM_TYPE_CONTROL, MWOS_STORAGE_NO, 1);
    // текущий статус счетчика
    MWOS_PARAM(2, status, PARAM_INT8, PARAM_TYPE_CONTROL, MWOS_STORAGE_NO, 1);
    // один шаг энкодера
    MWOS_PARAM(3, step, PARAM_FLOAT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // минимум на счетчике
    MWOS_PARAM(4, minimum, PARAM_INT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // максимум на счетчике
    MWOS_PARAM(5, maximum, PARAM_INT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // таймаут для отправки текущей позиции [сек/15]
    MWOS_PARAM(6, sendTime, PARAM_UINT16, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // тип энкодера
    MWOS_PARAM(7, type, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    MWOSCounter(MWOSCounterType counterType = CT_SOFTWARE, char * unit_name = (char *) &MWOS_COUNTER_NAME)
        : MWOSModule(unit_name) {
        moduleType = MODULE_ENCODER;
        _type = counterType;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                _counterPos = loadValueInt(_counterPos, p_counter, 0);
                _maximum = loadValueInt(_maximum, p_maximum, 0);
                _minimum = loadValueInt(_minimum, p_minimum, 0);
                _sendTime = loadValueInt(_sendTime, p_sendTime, 0);
                _step = loadValueDouble(_step, p_step, 0);
                _type = (MWOSCounterType)loadValueInt((int64_t)_type, p_type, 0);

                sendTimeout.start(_sendTime);
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_counter:
                        StartCount(data.toInt());
                        break;
                    case id_position:
                        StartPos(data.toDouble());
                        break;
                    case id_status: {
                        int8_t cmd = data.toInt();
                        switch (cmd) {
                            case 0: StartCount(_counterPos); break;
                            case 1: StartCount(0); break;
                            case 2: Pause(); break;
                            case 3: Resume(); break;
                            default: break;
                        }
                    } break;
                    case id_step:
                        _step = data.toDouble();
                        SetParamChanged(&p_step);
                        break;
                    case id_minimum:
                        _minimum = data.toInt();
                        SetParamChanged(&p_minimum);
                        break;
                    case id_maximum:
                        _maximum = data.toInt();
                        SetParamChanged(&p_maximum);
                        break;
                    case id_sendTimeDSec:
                        _sendTime = data.toInt();
                        sendTimeout.start(_sendTime);
                        SetParamChanged(&p_sendTimeDSec);
                        break;
                    case id_type:
                        _type = (MWOSCounterType)data.toInt();
                        SetParamChanged(&p_type);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                if (_direction != ED_STOP && _direction != ED_PAUSE) {
                    if (_sendTime > 0 && sendTimeout.isTimeout()) {
                        SetParamChanged(&p_counter);
                        SetParamChanged(&p_position);
                        SetParamChanged(&p_status);
                        sendTimeout.start(_sendTime);
                    }

                    if (getCounter() == targetPos && _callback != NULL) {
                        _callback();
                    }
                }
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_counter:
                        data.setValueInt(getCounter());
                        break;
                    case id_position:
                        data.setValueDouble(getPosition());
                        break;
                    case id_status:
                        data.setValueInt((int64_t)_direction);
                        break;
                    case id_step:
                        data.setValueDouble((double)_step);
                        break;
                    case id_minimum:
                        data.setValueInt(_minimum);
                        break;
                    case id_maximum:
                        data.setValueInt(_maximum);
                        break;
                    case id_sendTimeDSec:
                        data.setValueInt(_sendTime);
                        break;
                    case id_type:
                        data.setValueInt((int64_t)_type);
                        break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }
        MWOSModule::onEvent(modeEvent, data);
    }

    // --- Методы управления ---

    void StartPos(double newPosition) {
        MW_LOG_MODULE(this); MW_LOG(F("StartPos: ")); MW_LOG_LN(newPosition, 4);
        StartCount(round(newPosition / _step));
    }

    /**
    * Сбросить позицию к нулю и начать отсчет заново.
    */
    void StartCount(uint32_t newCountPos) {
        MW_LOG_MODULE(this); MW_LOG(F("StartCount: ")); MW_LOG_LN(newCountPos);
        _counterPos = newCountPos;
        timeoutStep = 0;

        if (_type == CT_TIMEOUT) {
            timeStep = millis();
            _direction = ED_FORWARD;
        } else {
            _direction = ED_STOP;
            timeStep = 0;
        }

        sendTimeout.start(_sendTime);

        SetParamChanged(&p_counter);
        SetParamChanged(&p_position);
        SetParamChanged(&p_status);
    }

    /**
    * На паузу.
    */
    void Pause() {
        if (_direction == ED_PAUSE) return;

        if (_type == CT_TIMEOUT) {
            _counterPos += timeoutMS();
            saveValueInt(_counterPos, p_counter, 0);
        }

        MW_LOG_MODULE(this); MW_LOG(F("Pause: ")); MW_LOG_LN(_counterPos);
        _direction = ED_PAUSE;

        SetParamChanged(&p_position);
        SetParamChanged(&p_status);
        SetParamChanged(&p_counter);
    }

    /**
    * Снять с паузы.
    */
    void Resume() {
        if (_direction != ED_PAUSE) return;

        if (_type == CT_TIMEOUT) {
            timeStep = millis();
            _direction = ED_FORWARD;
        } else {
            _direction = ED_STOP;
            timeStep = 0;
        }

        MW_LOG_MODULE(this); MW_LOG(F("Resume: ")); MW_LOG_LN(_counterPos);
        sendTimeout.start(_sendTime);

        SetParamChanged(&p_position);
        SetParamChanged(&p_status);
        SetParamChanged(&p_counter);
    }

    /**
    * Вернуть позицию энкодера без учета шага
    */
    int32_t getCounter() {
        if (_type == CT_TIMEOUT) return _counterPos + timeoutMS();
        return _counterPos;
    }

    /**
    * Вернуть позицию энкодера с учетом шага
    */
    double getPosition() {
        return ((double)getCounter()) * _step;
    }

    /**
    * Установить вызов функции по достижении целевой позиции
    */
    void setPosEvent(int32_t pos, voidFuncPtr callback) {
        _callback = callback;
        targetPos = pos;
    }

protected:
    #pragma pack(push, 1)
    voidFuncPtr _callback = NULL;
    int32_t targetPos = INT32_MAX;
    uint32_t timeStep = 0;
    uint32_t timeoutStep = 0;
    int32_t _counterPos = 0;
    int32_t _minimum = INT32_MIN;
    int32_t _maximum = INT32_MAX;
    float_t _step = 1;
    uint16_t _sendTime = 0;
    MWOSCounterType _type;
    MWOSEncoderDirection _direction = ED_STOP;
    MWTimeout<uint16_t,15,true> sendTimeout;
    #pragma pack(pop)

    uint32_t timeoutMS() {
        uint32_t nowTime = millis();
        if (nowTime > timeStep) timeoutStep = nowTime - timeStep;
        else timeoutStep = UINT32_MAX - timeStep + nowTime;
        return timeoutStep;
    }
};

#endif
