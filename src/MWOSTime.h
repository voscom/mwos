#ifndef MWOS3_MWOSTIME_H
#define MWOS3_MWOSTIME_H
/***
 * Модуль учета времени
 * Учитывает время [сек] от определенного события (по умолчанию - от старта контроллера)
 *
 *--Для stm32 поддерживается энергонезависимый таймер (часы реального времени на внешнем кварце 32768 Hz)
 */

#include <time.h>


class MWOSTime : public MWOSModule {
public:

    /**
     * Возвращает сколько миллисекунд прошло с момента _fromMSec
     * учитывает возможность переполнения таймера за это время
     */
    static uint32_t mSecFromTime(uint32_t _fromMSec) {
        return mSecFromTime(_fromMSec,millis());
    }
    static uint32_t mSecFromTime(uint32_t _fromMSec,uint32_t _nowMSec) {
        uint32_t _deltaMSec=_nowMSec-_fromMSec;
        if (_nowMSec<_fromMSec) _deltaMSec=0UL - _fromMSec + _nowMSec; // если было переполнение
        return _deltaMSec;
    }

#pragma pack(push,1)
    uint64_t _uptime=0; // время с момента включения (мСек=сек/1000)
    uint32_t _fromMSec=0;
    uint64_t _now=0;
    uint8_t _countSkip=0;
#pragma pack(pop)

    // описание параметров
    MWOS_PARAM(0, time, mwos_param_uint64, mwos_param_realtime+mwos_param_option, mwos_param_storage_no, 1);
    MWOS_PARAM(1, uptime, mwos_param_uint32, mwos_param_realtime+mwos_param_option, mwos_param_storage_no, 1);
    MWOS_PARAM(2, restarts, mwos_param_uint16, mwos_param_option, mwos_param_storage_rtc, 1);

    MWOSTime() : MWOSModule((char *) F("time")) {
        moduleType=ModuleType::MODULE_TIME;
        AddParam(&p_time);
        AddParam(&p_uptime);
        AddParam(&p_restarts);
    }

    virtual void onInit() {
        // увеличим количество перезагрузок
        uint16_t restartCount=loadValue(0,&p_restarts,0)+1;
        saveValue(restartCount,&p_restarts,0);
        SetParamChanged(&p_restarts,0,true);
        MW_LOG(F("Restart count: ")); MW_LOG_LN(restartCount);
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        _countSkip--;
        if (_countSkip!=0) return; // 1 из 255
        uint32_t _nowMSec=millis();
        uint32_t _deltaMSec=mSecFromTime(_fromMSec,_nowMSec);
        if (_deltaMSec>3600000UL) {
            _fromMSec=_nowMSec;
            _uptime+=_deltaMSec;
            SetParamChanged(&p_time, 0, true);
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 0: return getTime();
            case 1: return getUptime();
        }
        return MWOSModule::getValue(param,arrayIndex);
    }

    virtual void setValue(int64_t value, MWOSParam * param, int16_t arrayIndex) {
        if (param==&p_time) { // команда установки времени
            setTime(value);
        }
        else MWOSModule::setValue(value,param,arrayIndex);
    }

    uint64_t getUptime() {
        return (_uptime+mSecFromTime(_fromMSec))/1000UL;
    }

    uint64_t getUptimeMS() {
        return _uptime+(uint64_t) mSecFromTime(_fromMSec);
    }

    uint64_t getTime() {
        return _now+getUptime();
    }

    uint64_t getTimeMS() {
        return ((uint64_t) _now)*1000UL+getUptime();
    }

    void setTime(uint64_t unixTime) {
        MW_LOG_MODULE(this); MW_LOG(F("setTime: ")); MW_LOG_LN((uint32_t) unixTime);
        _now=unixTime-(_uptime/1000UL); // время на момент старта контроллера
        SetParamChanged(&p_time, 0, true);
    }

    bool IsTimeSetting() {
        return _now>0;
    }

    /**
     * Возвращает секунды с начала суток
     */
    uint32_t dailySec() {
        return (getTime() % 86400UL); // так как время timestamp считается с полуночи, то это просто остаток от деления на количество секунд в сутках
    }

    /**
     * Возвращает минуты с начала суток
     */
    uint16_t dailyMin() {
        return dailySec()/60;
    }

    /**
     * Текущее время попадает в диапазон
     * @timeFromMin время (в минутах от начала суток) начала диапазона
     * @timeToMin время (в минутах от начала суток) конца диапазона
     * @return Текущее время попадает в заданный диапазон
    */
    bool timeIn(uint16_t timeFromMin,uint16_t timeToMin) {
        uint16_t timeMin=dailyMin();
        if (timeFromMin>timeToMin) {
            return ((timeMin>=timeFromMin) ||  (timeMin<=timeToMin));
        }
        return ((timeMin>=timeFromMin) &&  (timeMin<=timeToMin));
    }

    void printTimeTo(Print * toStream, time_t t=0) {
        if (t==0) t = getTime();
        struct tm *tm = localtime(&t);
        uint8_t hours=tm->tm_hour;
        uint8_t sec=tm->tm_sec;
        uint8_t min=tm->tm_min;
        if (hours<10) toStream->print('0');
        toStream->print(hours);
        toStream->print(':');
        if (min<10) toStream->print('0');
        toStream->print(min);
        toStream->print(':');
        if (sec<10) toStream->print('0');
        toStream->print(sec);
    }

    void printDaysTo(Print * toStream, time_t t=0) {
        if (t==0) t = getUptime();
        uint32_t days=t/86400UL;
        toStream->print(days);
        toStream->print('.');
        printTimeTo(toStream,t);
    }

    void printDateTo(Print * toStream, time_t t=0, char dline='.') {
        if (t==0) t = getTime();
        struct tm *tm = localtime(&t);
        uint8_t year=tm->tm_year;
        uint8_t mon=tm->tm_mon;
        uint8_t day=tm->tm_mday;
        toStream->print(year);
        toStream->print(dline);
        if (mon<10) toStream->print('0');
        toStream->print(mon);
        toStream->print(dline);
        if (day<10) toStream->print('0');
        toStream->print(day);
    }


};


#endif //MWOS3_MWOSTIME_H
