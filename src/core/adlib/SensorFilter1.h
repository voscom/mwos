#ifndef SensorFilter1_H
#define SensorFilter1_H

#include <core/adlib/SensorFilter.h>

class SensorFilter1 : public SensorFilter {
private:
    double x_kf = 0;
    double P_kf = 1.0f;
public:

    // Сброс алгоритма (перед первым значением)
    void reset() {
        x_kf=0;
        P_kf=1.0f;
    }

    void update(double measurement, uint32_t currentTimeMillis) {
        // Прогноз
        P_kf += Q;
        // Коррекция
        double K = P_kf / (P_kf + R);
        x_kf += K * (measurement - x_kf);
        P_kf *= (1.0f - K);
    }

    double getCorrected() {
        return x_kf;
    }

};

#endif
