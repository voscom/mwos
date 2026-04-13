#ifndef SensorFilter_H
#define SensorFilter_H

class SensorFilter {
private:
    double x_kf = 0;
    double P_kf = 1.0f;

public:
    double Q = 0.01f;      // Шум процесса
    double R = 0.01f;     // Шум измерения
    double tau = 15.0;     // Примерное время выхода на плато [сек]
    float * filterOptions; // дополнительные настройки фильтра (массив на FilterOptionsCount значений типа float)

    bool needFinish=false; // признак, что расчеты закончены и необходимо остановить замеры (может быть актуально для поиска плато)

    // Сброс алгоритма (перед первым значением)
    virtual void reset() {
        x_kf=0;
        P_kf=1.0f;
        needFinish=false;
    }

    virtual void update(double measurement, uint32_t currentTimeMillis) {
        // Прогноз
        P_kf += Q;
        // Коррекция
        double K = P_kf / (P_kf + R);
        x_kf += K * (measurement - x_kf);
        P_kf *= (1.0f - K);
    }

    virtual double getCorrected() {
        return x_kf;
    }

    // получить финальный результат после окончания замеров (когда датчик на паузе)
    virtual double getFinishResult() {
        return getCorrected();
    }

};

#endif
