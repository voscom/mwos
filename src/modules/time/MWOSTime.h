#ifndef MWOS3_MWOSTIME_H
#define MWOS3_MWOSTIME_H
/***
 * Модуль учета времени
 * Учитывает время [сек] от определенного события (по умолчанию - от старта контроллера)
 *
 * Команды
 * Сброс количества перезагрузок:
 time:restarts=0
 * Перезагрузка (пока только ESP32)
 time:restarts=1
 * Сброс настроек и перезагрузка (пока только ESP32)
 time:restarts=99
 *
 *
 *--Для stm32 поддерживается энергонезависимый таймер (часы реального времени на внешнем кварце 32768 Hz)
 */

#include "core/adlib/DateTime.h"

class MWOSTime : public MWOSModule {
public:

    // найти активный модуль времени
    static MWOSTime * getTimeModule() {
        return (MWOSTime *) mwos.FindChildByModuleType(MODULE_TIME);
    }

    // время unixtime по Гринвичу
    MWOS_PARAM(0, time, PARAM_UINT64, PARAM_TYPE_REALTIME+PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);
    MWOS_PARAM(1, uptime, PARAM_UINT32, PARAM_TYPE_REALTIME+PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);
    MWOS_PARAM(2, restarts, PARAM_UINT16, PARAM_TYPE_OPTIONS+PARAM_TYPE_READONLY, MWOS_STORAGE_RTC, 1);
    MWOS_PARAM(3, freemem, PARAM_UINT32, PARAM_TYPE_OPTIONS+PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // время сек от начала 2024 по Гринвичу
    MWOS_PARAM(4, time24, PARAM_UINT32, PARAM_TYPE_REALTIME+PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, 1);
    // таймзона (смещение от гринвича в четвертях часа (*15мин))
    MWOS_PARAM(5, timezone, PARAM_INT8, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

#pragma pack(push,1)
    uint64_t _uptimeMSec=0; // микросекунды [], на момент старта контроллера (задается при изменении времени, или при переполнении таймера millis)
    uint8_t _countSkip=0;
    int8_t _timezone=12; // по умолчанию МСК +3:00 (3*4)
    uint8_t _millisHigh=0; // таймер в millis перешел за середину
    uint8_t _uptimeHigh=0; // старший байт времени uptime
#pragma pack(pop)

    /**
     * Возвращает сколько миллисекунд прошло с момента _fromMSec
     * учитывает возможность переполнения таймера за это время
     */
    static uint32_t mSecFromTime(uint32_t fromMSec) {
        return mSecFromTime(fromMSec,millis());
    }
    static uint32_t mSecFromTime(uint32_t fromMSec,uint32_t nowMSec) {
        uint32_t _deltaMSec=nowMSec-fromMSec;
        if (nowMSec<fromMSec) _deltaMSec=0UL - fromMSec + nowMSec; // если было переполнение
        return _deltaMSec;
    }

    MWOSTime() : MWOSModule((char *) F("time")) {
        moduleType=MODULE_TIME;
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы.
     * Так же, вызывается при запросе значений и приходе новых данных.
     * @param modeEvent    Тип вызываемого системного события
     * @param data    Данные, передаваемые в событие, и возвращаемые из события (просто изменить data)
     */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) {
        switch (modeEvent) {
            case EVENT_INIT: {
                _timezone=loadValueInt(_timezone,p_timezone,0);
                uint16_t restartCount = loadValueInt(0, p_restarts, 0) + 1;
                saveValueInt(restartCount, p_restarts, 0);
                MW_LOG_MODULE(this); MW_LOG(F("restartCount: ")); MW_LOG_LN(restartCount);
                SetParamChanged(&p_restarts, 0, true);
                SetParamChanged(&p_freemem, 0, true);
            } break;
            case EVENT_UPDATE: {
                _countSkip--;
                if (_countSkip!=0) return; // 1 из 255
                uint32_t _nowMSec=millis();
                if ((_nowMSec & (uint32_t) 0x80000000)>0) _millisHigh=true;
                else if (_millisHigh) {
                    _uptimeMSec+=(uint64_t) 0x100000000; // учтем переполнение таймера
                    _uptimeHigh++;
                    _millisHigh=false;
                }
            } break;
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_time: setTime(data.getUInt64()); break;
                    case id_time24: setTime24(data.getUInt32()); break;
                    case id_restarts: {
                        uint16_t rc=data.toInt();
                        if (rc==1) resetController();
                        if (rc==0) {
                            saveValueInt(0, p_restarts, 0);
                            SetParamChanged(&p_restarts, 0, true);
                        }
                    } break;
                }
            } break;
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_time: data.setValueInt(getTime(false)); break;
                    case id_uptime: data.setValueInt(getUptime()); break;
                    case id_freemem: data.setValueInt(getFreeMemory()); break;
                    case id_time24: data.setValueInt(getTime24(false)); break;
                    case id_timezone: data.setValueInt(_timezone); break;
                }

            } break;
        }
        return MWOSModule::onEvent(modeEvent,data);
    }

     uint32_t getTime24(bool localtime) {
        return (uint32_t) (getTime(localtime)-((uint64_t) 1704067200));
    }

    uint64_t getUptime() {
        return getUptimeMS()/(uint64_t) 1000;
    }

    uint64_t getUptimeMS() {
        return (((uint64_t) _uptimeHigh) << 32) + (uint64_t) millis();
    }

    uint64_t getTime(bool localtime=true) {
        return getTimeMS(localtime) /(uint64_t) 1000;
    }

    uint64_t getTimeMS(bool localtime=true) {
        uint64_t res=_uptimeMSec+(uint64_t) millis();
        if (localtime) res+=((uint64_t) _timezone) * (uint64_t) 900000;
        return res;
    }

    void setTime(uint64_t unixTime) {
#ifdef MWOS_DEBUG
        uint64_t unixTimeDeb=unixTime;
        int64_t deltaTime=unixTimeDeb-getTime(false);
        if (deltaTime>3 || deltaTime<-3) { // только при изменении времени более 3 сек
            DateTime dt1(unixTimeDeb);
            MW_LOG_MODULE(this); MW_LOG(F("setUnixtime: ")); MW_LOG(unixTimeDeb); MW_LOG('='); MW_LOG_LN(dt1.timestamp(DateTime::timestampOpt::TIMESTAMP_FULL));
        }
#endif
        int64_t oldMSec=_uptimeMSec;
        _uptimeMSec=unixTime * (uint64_t) 1000;
        _uptimeMSec -= millis(); // время на момент старта контроллера
        SetParamChanged(&p_time, 0, true);
        if (abs(oldMSec-(int64_t) _uptimeMSec)>5000) { // если изменили время на 5 и более секунд
            mwos.mwosValue.param_id=id;
            mwos.dispatchEvent(EVENT_TIME_CHANGE,mwos.mwosValue);
        }
    }

    void setTime24(uint64_t time24) {
        setTime(time24+1704067200);
    }

    bool IsTimeSetting() {
        return _uptimeMSec>1704067200;
    }

    /**
     * Возвращает секунды с начала суток
     */
    uint32_t dailySec(time_t t=0) {
        if (t==0) t = getTime(true);
        return (t % 86400UL); // так как время timestamp считается с полуночи, то это просто остаток от деления на количество секунд в сутках
    }

    /**
     * Возвращает минуты с начала суток
     */
    uint16_t dailyMin(time_t t=0) {
        return dailySec(t)/60;
    }

    /**
     * Возвращает часы с начала суток
     */
    uint16_t dailyHour(time_t t=0) {
        return dailySec(t)/3600;
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

    String toString(const char * format="YYYY-MM-DD hh:mm:ss", uint64_t unixTime=0) {
        if (unixTime==0) unixTime=getTime(true);
        DateTime dt(unixTime);
        uint16_t sizBuff=strlen(format);
        char buff[sizBuff+1];
        for (uint16_t i=0; i<sizBuff; i++) buff[i]=format[i];
        buff[sizBuff]=0;
        String res=String(dt.toString(buff));

        return res;
    }

};


#endif //MWOS3_MWOSTIME_H
