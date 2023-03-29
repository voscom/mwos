#ifndef MWTimeout_h
#define MWTimeout_h

#include <Arduino.h>


class MWTimeout {
public:

	/**
	 * Начать отсчет времени [sec/10]
	 */
	void start(uint16_t timeoutDSec=0) {
		ms_from=millis();
		ms_to=ms_from+timeoutDSec*100UL;
	}

	/**
	 * Начать отсчет времени [sec/1000]
	 */
	void startMS(uint32_t timeoutMSec=0) {
		ms_from=millis();
		ms_to=ms_from+timeoutMSec;
	}

	/**
	 * Остановить отсчет времени
	 */
	void stop() {
		ms_from=0;
		ms_to=0;
	}

	/**
	 * Сколько микросекунд (сек/1000) прошло с момента старта этого таймаута
	 */
	uint32_t msFromStart() {
		unsigned long _nowMSec=millis();
		unsigned long _deltaMSec=_nowMSec-ms_from;
		if (_nowMSec<ms_from) _deltaMSec=0UL - ms_from + _nowMSec; // если было переполнение
		return _deltaMSec;
	}

	/**
	 * Сколько децисекунд (сек/10) прошло с момента старта этого таймаута
	 */
	uint16_t dsFromStart() {
		uint32_t ms=msFromStart();
		return (uint16_t) ms/100UL;

	}

	/**
	 * Отсчет начинали ранее ?
	 */
	bool isStarted() {
		return (ms_from>0) || (ms_to>0);
	}

	/**
	 * Заданный таймаут вышел ?
	 */
	bool isTimeout() {
		uint32_t nowTime=millis();
		return  (((ms_to>=ms_from) && ((nowTime>=ms_to) || (nowTime<ms_from)))  // время прошло или таймер переполнился
				|| ((ms_to<ms_from) && (nowTime>=ms_to) && (nowTime<ms_from))); // уже засекли с переполнением - и время пришло, когда таймер уже переполнился
	}

private:
#pragma pack(push,1)
	uint32_t ms_from=0;
	uint32_t ms_to=0;
#pragma pack(pop)

};

#endif
