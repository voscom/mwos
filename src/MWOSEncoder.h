#ifndef MWOS3_MWOSENCODER_H
#define MWOS3_MWOSENCODER_H
#include "core/MWOSModule.h"
#if defined(ESP32) || defined(ESP8266)
#include <FunctionalInterrupt.h>
#endif
#include <stdlib.h>

//#define timeoutStopMKS 50000 // таймаут, через который энкодер считать остановленным [микросекунд] [1 / 1 000 000 сек] если не задано то этот функционал отключен

/***
 * Модуль энкодера
 * поддерживает программный режим и режим на прерываниях
 * поддержка аппаратного режима в потомках
 * может измерять частоту
 *
 * есть 2 способа замера частоты:
 * 1. Время между перепадами фронтов
 * 2. Количество тиков за таймаут
 *
 * Параметры:
 * 0 - текущая позиция (можно записывать)
 * 1 - частота на энкодере. Вместо записи - команда.
 *      0 - restart (начать заново замер скорости)
 * 2 - направление движения (только чтение)
 *
 * и далее настройки (см параметры)
 *
 */
class MWOSEncoder : public MWOSModule {
public:

    enum MWOSEncoderDirection {
        BACKWARD=0,
        FORWARD=1,
        STOP=2
    };

    enum MWOSEncoderType {
        USE_SOFTWARE=0,
        USE_INTERRUPT=1,
        USE_HARDWARE=2
    };

#pragma pack(push,1)
    int32_t _position=0;
    uint8_t _direction=MWOSEncoderDirection::STOP;
    uint8_t _pinA=255;
    uint8_t _pinB=255;
    uint8_t _pinRef=255;
    int32_t _minimum=INT32_MIN;
    int32_t _maximum=INT32_MAX;
    int16_t _filter=0;
    int16_t _sendTime=200;
    int32_t _stopTime=60;
    uint8_t _type:2;
    uint8_t _alter:1;
    uint8_t _needSpeed:1;
    uint8_t _invertDirection:1;
    uint8_t _pull:2;
    uint8_t _invertSignal:1;
    uint32_t _stopTimeMKS=0;

    // текущая позиция энкодера
    uint32_t timeStep=0;
    uint32_t timeoutStep=0;

    bool initedSoft=false;
#pragma pack(pop)

    // текущая позиция (можно записывать)
    MWOS_PARAM(0, position, mwos_param_int32, mwos_param_sensor, mwos_param_storage_rtc, 1);
    // частота на энкодере. Вместо записи - команда сброса начала замера частоты.
    MWOS_PARAM(1, speed, mwos_param_int32, (MWOSParamGroup) (mwos_param_sensor + mwos_param_readonly), mwos_param_storage_no, 1);
    // направление движения (0-назад, 1-вперед, 2-останов)
    MWOS_PARAM(2, direction, mwos_param_bits2, (MWOSParamGroup) (mwos_param_sensor + mwos_param_readonly), mwos_param_storage_no, 1);
    // порт A
    MWOS_PARAM(3, pinA, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, 1);
    // порт B
    MWOS_PARAM(4, pinB, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, 1);
    // порт раференции
    MWOS_PARAM(5, pinRef, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, 1);
    // минимум
    MWOS_PARAM(6, minimum, mwos_param_int32, mwos_param_option, mwos_param_storage_eeprom, 1);
    // максимум
    MWOS_PARAM(7, maximum, mwos_param_int32, mwos_param_option, mwos_param_storage_eeprom, 1);
    // длина минимального сигнала [mkS] (сек/1.000.000), ниже которого считаем, что это помеха (всключение фильтра может отрицательно влиять на производительность)
    MWOS_PARAM(8, filter, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // [сек/1000] через какое время выдавать значения энкодера на сервер, в случае изменения значений
    MWOS_PARAM(9, sendTime, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // [mS] (сек/1000) минимальная длина импульса энкодера для подсчета скорости (импульс длинее этого времени считаем остановкой энкодера - скорость 0)
    MWOS_PARAM(10, stopTime, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // тип энкодера: 0-программный, 1-на прерываниях, 2-аппаратный (только для потомков)
    MWOS_PARAM(11, type, mwos_param_bits2, mwos_param_option, mwos_param_storage_eeprom, 1);
    // альтернативный режим работы энкодера: подсчет импульсов на A - плюс, на B - минус (ВНИМАНИЕ!!! пока только для PCNT)
    MWOS_PARAM(12, alter, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // считать скорость (подсчет скорости может отрицательно влиять на производительность)
    MWOS_PARAM(13, needSpeed, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // инверсия направления
    MWOS_PARAM(14, invertDirection, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // подтяжка всех портов (0-нет, 1 - на 0, 2 - на питание)
    MWOS_PARAM(15, pull, mwos_param_bits2, mwos_param_option, mwos_param_storage_eeprom, 1);
    // инверсия уровней портов
    MWOS_PARAM(16, invertSignal, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);

#ifdef STM32_MCU_SERIES
    PinName _pinBName;
#endif

    MWOSEncoder() : MWOSModule((char *) F("encoder")) {
        AddParam(&p_position);
        AddParam(&p_speed);
        AddParam(&p_direction);
        AddParam(&p_pinA);
        AddParam(&p_pinB);
        AddParam(&p_pinRef);
        AddParam(&p_minimum);
        AddParam(&p_maximum);
        AddParam(&p_filter);
        AddParam(&p_sendTime);
        AddParam(&p_stopTime);
        AddParam(&p_type);
        AddParam(&p_alter);
        AddParam(&p_needSpeed);
        AddParam(&p_invertDirection);
        AddParam(&p_pull);
        AddParam(&p_invertSignal);
        _pull=0;
        _type=0;
        _alter=0;
        _needSpeed=0;
        _invertDirection=0;
        _invertSignal=0;
    }

    MWOSEncoder(uint8_t pinA,uint8_t pinB,uint8_t pinRef=255) : MWOSEncoder() {
        _pinA = pinA;
        _pinB = pinB;
        _pinRef = pinRef;
    }

    virtual void onInit() {
        _position=loadValue(_position, &p_position);
        // настроим порты
        _pull=loadValue(_pull, &p_pull);
        _pinA=loadValue(255, &p_pinA);
        if (_pinA<255) mwos.pin(_pinA)->mode(false, _pull);
        _pinB=loadValue(255, &p_pinB);
        if (_pinB<255) {
            mwos.pin(_pinB)->mode(false, _pull);
#ifdef STM32_MCU_SERIES
            _pinBName=digitalPinToPinName(_pinB);
#endif
        }
        _pinRef=loadValue(255, &p_pinRef);
        if (_pinRef<255) mwos.pin(_pinRef)->mode(false, _pull);
        // прочитаем все настройки
        _maximum=loadValue(_maximum, &p_maximum);
        _minimum=loadValue(_minimum, &p_minimum);
        _filter=loadValue(_filter, &p_filter);
        _sendTime=loadValue(_sendTime, &p_sendTime);
        _stopTime=loadValue(_stopTime, &p_stopTime);
        _stopTimeMKS=_stopTime*1000UL;
        _type=loadValue(_type, &p_type);
        _alter=loadValue(_alter, &p_alter);
        _needSpeed=loadValue(_needSpeed, &p_needSpeed);
        _invertDirection=loadValue(_invertDirection, &p_invertDirection);
        _invertSignal=loadValue(_invertSignal, &p_invertSignal);
        initedSoft= false;
        if (_pinA<255 && _pinB<255) {
            initedSoft=(_type == MWOSEncoderType::USE_SOFTWARE);
            if (_type==MWOSEncoderType::USE_INTERRUPT) {
                detachInterrupt(digitalPinToInterrupt(_pinA));
                if (_invertSignal ^ _invertDirection) {
                    attachInterrupt(digitalPinToInterrupt(_pinA),std::bind(&MWOSEncoder::encodeStep, this), FALLING ); // с HIGH на LOW
                } else {
                    attachInterrupt(digitalPinToInterrupt(_pinA),std::bind(&MWOSEncoder::encodeStep, this), RISING); // с LOW на HIGH
                }
            }
        }
    }

    virtual void setValue(int64_t v, MWOSParam * param, int16_t arrayIndex= 0) {
        MW_LOG_MODULE(this,arrayIndex); MW_LOG(F("setValue: ")); MW_LOG_LN((int32_t) v);
        if (param==&p_speed && v==0) restart();
        else {
            MWOSModule::setValue(v,param,arrayIndex);  // сохраним в хранилище
            onInit(); // обновим настройки
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex=0) {
        switch (param->id) {
            case 0: return _position;
            case 1: return getSpeed();
            case 2: return _direction;
        }
        return MWOSModule::getValue(param,arrayIndex);
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (initedSoft) {
            bool nowPortA=digitalRead(_pinA);
            if (lastPortA!=nowPortA) { // перепад фронта
                if ((_invertSignal ^ _invertDirection) && (!nowPortA)) encodeStep(); // с HIGH на LOW
                else if (nowPortA) encodeStep();  // с LOW на HIGH
                lastPortA=nowPortA;
            }
        }
    }

    virtual void restart() {
        MW_LOG_LN(F("Encoder restart"));
        timeStep=0;
        timeoutStep=0;
        _direction=MWOSEncoderDirection::STOP;
    }

    /**
     * Возвращает текущую частоту вращения энкодера
     * Если не было импульсов более timeoutStopMKS - считает это остановкой энкодера
     * @return
     */
    int32_t getSpeed() {
        if (!_needSpeed || timeoutStep==0 || _direction==MWOSEncoderDirection::STOP) return 0;
        if (_stopTime>0) {
            uint32_t nowTime=micros();
            if (nowTime>timeStep) {
                if (nowTime-timeStep>_stopTimeMKS) {
                    _direction=MWOSEncoderDirection::STOP;
                    return 0;
                }
            } else {
                if (UINT32_MAX-timeStep+nowTime>_stopTimeMKS) {
                    _direction=MWOSEncoderDirection::STOP;
                    return 0;
                }
            }
        }
        return 1000000L/timeoutStep;
    }

    /***
     * Установить вызов функции по достижении целевой позиции
     * @param pos
     * @param callback
     */
    void setPosEvent(int32_t pos,std::function<void(void)> callback) {
        _callback = callback;
        targetPos=pos;
    }

    void encodeStep() { // вызывается по прерыванию от ножки А (или из программы)
#ifdef STM32_MCU_SERIES
        bool nowDirection=digitalReadFast(_pinBName); // направление движения
#else
        bool nowDirection=digitalRead(_pinB); // направление движения
#endif
        if (nowDirection ^ _invertDirection) _position++;
        else _position--;
        if (_position==targetPos && _callback!=NULL) _callback(); // вызовем заданную функцию на целевой позиции
        // расчет времени импульса
        if (_needSpeed) {
            if (_direction==nowDirection) { // это не первый шаг в этом направлении
                uint32_t nowTime=micros();
                if (nowTime>timeStep) timeoutStep=nowTime-timeStep;
                else timeoutStep=UINT32_MAX-timeStep+nowTime; // переполнение счетчика
                timeStep=nowTime;
            } else { // это первый шаг в этом направлении
                _direction = nowDirection;
                timeoutStep=0;
                timeStep=micros();
            }
        }
    }

    void encodeRef() {

    }

private:
    std::function<void(void)> _callback=NULL;
    int32_t targetPos=INT32_MAX;
    bool lastPortA=false;
};


#endif //MWOS3_MWOSENCODER_H
