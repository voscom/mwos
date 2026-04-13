#ifndef MWTimeout_h
#define MWTimeout_h

#include <Arduino.h>
#include <type_traits> // для std::make_signed
#include <cmath>       // для std::round

class MWTimeoutBase {
protected:
	static uint32_t nowTime; // Время всегда храним в мс (32 бита)
public:
	/**
	 * Обновление текущего времени для всех таймеров. После этого, можно вызывать методы таймеров с refresh = false
	 */
	static void refresh() { nowTime = millis(); }
};
uint32_t MWTimeoutBase::nowTime = 0;

/***
 * Таймаут от старта до указанного значения. Занимает в ОЗУ только заданный размер в байтах (1,2,4).
 * @tparam classTime	Переменную какого типа выделить для хранения таймаута (uint8_t, uint16_t, uint32_t).
 * @tparam accuracy	Точность (1 - секунды, 10 - 1/10 секунды, 100 - 1/100 секунды, 1000 - 1/1000 секунды)
 * @tparam divOptimization	Если включено, деление будет заменено на бинарный сдвиг. Работает быстро, но точность таймера может упасть.
 * Актуально для контроллеров без аппаратного деления. При accuracy=1000 можно не включать - деления не будет.
 */
template<class classTime=uint16_t, uint16_t accuracy=1000, bool divOptimization = false>
class MWTimeout : public MWTimeoutBase {
private:

	// Константы для оптимизации сдвигом
	static constexpr uint32_t rawDivisor = (accuracy > 0 && accuracy <= 1000) ? (1000 / accuracy) : 1;

	// Находим ближайшую степень двойки (округление)
	static constexpr uint8_t getNearestShift(uint32_t n) {
		return (n <= 1) ? 0 :
			   ((1UL << getShift(n)) * 3 / 2 > n) ? getShift(n) : getShift(n) + 1;
	}
	// Находим степень двойки логарифм вниз
	static constexpr uint8_t getShift(uint32_t n, uint8_t shift = 0) {
		return (n <= 1) ? shift : getShift(n >> 1, shift + 1);
	}
	// Итоговое значение сдвига теперь точнее
	static constexpr uint8_t shiftVal = getNearestShift(rawDivisor);

public:
    classTime ms_start=0;

	/**
	 * Получить текущее время с заданной точностью.
	 * @param refresh	Обновить время (иначе - возвращает последнее)
	 * @return [секунды/точность]
	 */
	static classTime getNowTime(bool refresh = true) {
		static_assert(accuracy <= 1000, "Accuracy must be <= 1000");
		if (refresh || nowTime == 0) {
			nowTime = millis(); // Обновляем общие МИЛЛИСЕКУНДЫ
		}
		// Локально возвращаем время с нужной точностью
		if (accuracy == 1000) {
			return (classTime)nowTime;
		}
		if (divOptimization) {
			return (classTime)(nowTime >> shiftVal);
		} else {
			return (classTime)(nowTime / rawDivisor);
		}
	}

	MWTimeout() {}
	MWTimeout(int32_t time, bool refresh=true) { if (time!=0) start(time,refresh); }
	MWTimeout(float timeSec, bool refresh=true) { if (timeSec!=0) startSec(timeSec, refresh); }

	/**
	 * Начать отсчет до заданного времени.
     * @param time Время до таймаута, задается в [секундах/точность]
	 * @param refresh	Обновить время
     */
    void start(int32_t time=0, bool refresh=true) {
		ms_start=getNowTime(refresh)+time;
		if (ms_start==0) ms_start=1;
	}

	/**
	 * Начать отсчет до заданного времени в миллисекундах.
     * @param timeMSec	Время до таймаута, Миллисекунды [секунда/1000]
	 * @param refresh	Обновить время
     */
    void startMS(int32_t timeMSec=0, bool refresh=true) {
		if (divOptimization) {
			ms_start = getNowTime(refresh) + (classTime)(timeMSec >> shiftVal);
		} else {
			ms_start = getNowTime(refresh) + (classTime)(timeMSec / rawDivisor);
		}
		if (ms_start==0) ms_start=1;
	}

	/**
	 * Начать отсчет до заданного времени в секундах.
     * @param timeSec Секунды, дробью
	 * @param refresh	Обновить время
     */
    void startSec(float timeSec=0, bool refresh=true) {
		ms_start=getNowTime(refresh)+(int32_t) round(timeSec * (float) accuracy);
		if (ms_start==0) ms_start=1;
	}

	/**
	 * Остановить отсчет времени
	 */
	void stop() {
		ms_start=0;
	}

	/**
	 * Отсчет начинали ранее ?
	 */
	bool isStarted() {
		return ms_start>0;
	}

	/**
	 * Сколько времени (сек/точность) прошло с момента окончания этого таймаута
     * @param extra Дополнительный таймаут, от запомненного (сек/точность).
	 * @param refresh	Обновить время
     * @return	Время с момента таймаута (с заданной точностью). Если таймаут еще не вышел - время отрицательное.
     */
	int32_t timeout(int32_t extra = 0, bool refresh=true) {
		using signedTime = typename std::make_signed<classTime>::type;
		classTime ms_now = getNowTime(refresh);
		// ms_start у нас — это время дедлайна (будущее).
		// Вычисляем разницу: "сейчас" минус "дедлайн".
		// Если "сейчас" чуть-чуть не дошло до "дедлайна",
		// результат будет отрицательным (в пределах половины диапазона).
		classTime diff = ms_now - ms_start;
		// Приводим к знаковому типу.
		// Для uint8_t: если diff > 128, это станет отрицательным числом.
		return (int32_t)((signedTime)diff - (signedTime)extra);
	}

	/**
	 * Сколько прошло с момента окончания этого таймаута
	 * @param extraMS Дополнительный таймаут, от запомненного в миллисекундах [сек/1000].
	 * @param refresh	Обновить время
	 * @return	Время с момента таймаута (с заданной точностью). Если таймаут еще не вышел - время отрицательное.
	 */
	int32_t timeoutMS(int32_t extraMS = 0, bool refresh=true) {
		// Сначала получаем разницу в тиках, переводим в мс, затем добавляем дельту мс
		int32_t diff = timeout(0, refresh);
		return divOptimization ? (diff << shiftVal) + extraMS : (diff * rawDivisor) + extraMS;
	}

	/**
	 * Сколько секунд прошло с момента окончания этого таймаута
	 * @param extraSec Дополнительный таймаут, от запомненного в секундах
	 * @param refresh	Обновить время
	 * @return	Секунд с момента таймаута. Если таймаут еще не вышел - время отрицательное.
	 */
	float timeoutSec(float extraSec = 0, bool refresh=true) {
		return ((float)timeout(0,refresh) / (float)accuracy) + extraSec;
	}

	/**
	 * Заданный таймаут вышел ?
	 * @param extra Дополнительный таймаут (с заданной точностью), от запомненного.
	 * @param refresh	Обновить время
	 * @return Таймаут вышел. Или таймер не был запущен.
	 */
	bool isTimeout(int32_t extra=0, bool refresh=true) {
		return ms_start == 0 || timeout(extra,refresh)>=0;
	}

	/**
	 * Заданный таймаут вышел ?
	 * @param extraSec Дополнительный таймаут секунд, от запомненного.
	 * @param refresh	Обновить время
	 * @return Таймаут вышел. Или таймер не был запущен.
	 */
	bool isTimeoutSec(float extraSec=0, bool refresh=true) {
		return ms_start == 0 || timeoutSec(extraSec,refresh)>=0;
	}

	/**
	 * Заданный таймаут вышел ?
	 * @param extraMS Дополнительный таймаут миллисекунд, от запомненного.
	 * @param refresh	Обновить время
	 * @return Таймаут вышел. Или таймер не был запущен.
	 */
	bool isTimeoutMS(float extraMS=0, bool refresh=true) {
		return ms_start == 0 || timeoutMS(extraMS,refresh)>=0;
	}

};

#endif
