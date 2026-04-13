#ifndef MWOS35_MWOSSENSORFILTER_H
#define MWOS35_MWOSSENSORFILTER_H

#include "core/MWOSModule.h"
#include "MWOSSensorAnalog.h"
#include "core/adlib/SensorFilter.h"
#include "core/adlib/SensorFilter1.h"
#include "core/adlib/SensorFilter2.h"
#include "core/adlib/SensorFilter3.h"
#include "core/adlib/SensorFilter4.h"

#ifndef FilterOptionsCount
// размер массива дополнительных настроек фильтров
#define FilterOptionsCount 5
#endif

// режим расширенного фильтра
enum ExtendedFilterMode {
    // фильтр отключен
    EFM_None = 0,
    // фильтр 1
    EFM_Filter1 = 1,
    // фильтр 2
    EFM_Filter2 = 2,
    // фильтр 3
    EFM_Filter3 = 3,
    // фильтр 4
    EFM_Filter4 = 4,
};

/**
* Расширение аналоговых датчиков с коррекцией показаний и дробными значениями (Версия MWOS3.5)
*
* Добавлен алгоритм простого сглаживания Калмана.
* Добавлен Расширенный фильтр Калмана (EKF) с предсказанием плато.
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - загрузка настроек при старте.
*   EVENT_CHANGE - реакция на изменение конкретного параметра.
*   EVENT_UPDATE - тиковая обработка (чтение датчиков, фильтрация).
*   EVENT_GET_VALUE - возврат значений через switch.
*/
template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorFilter : public MWOSSensorAnalog<sensorsCount> {
public:
    // коэффициент умножения для каждого датчика
    float _coef[sensorsCount];
    // коэффициент для вычитания для каждого датчика
    float _subCoef[sensorsCount];
    // включено сглаживание для каждого датчика
    ExtendedFilterMode _filter[sensorsCount];
    // фильтр сглаживания для каждого датчика
    SensorFilter * sensorFilter[sensorsCount];
    // настройки для фильтров (общие для всех)
    float _filterOptions[FilterOptionsCount];

    // --- Объявление параметров (автоматическая регистрация) ---

    // результат с учетом коэффициентов и фильтров
    MWOS_PARAM(30, result, PARAM_DOUBLE64, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, sensorsCount);
    // коэффициент умножения
    MWOS_PARAM(31, coef, PARAM_FLOAT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);
    // коэффициент для вычитания
    MWOS_PARAM(32, subCoef, PARAM_FLOAT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);
    // предсказание плато по расширенному фильтру Калмана (если EKF не включен, то равен параметру result)
    MWOS_PARAM(34, predictedEKF, PARAM_DOUBLE64, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NO, sensorsCount);
    // включение алгоритма сглаживания Калмана
    MWOS_PARAM_FF(35, filter, PARAM_BITS3, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount,
                  "Выкл;Фильтр1;Фильтр2;Фильтр3;Фильтр4");
    // Калман Q - шум процесса (чем выше, тем больше доверия новым данным)
    MWOS_PARAM(36, kalmanQ, PARAM_DOUBLE64, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);
    // Калман R - Шум измерений (чем выше, тем больше сглаживание)
    MWOS_PARAM(37, kalmanR, PARAM_DOUBLE64, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);
    // предполагаемое время выхода на плато (сек)
    MWOS_PARAM(38, kalmanTau, PARAM_FLOAT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, sensorsCount);
    // признак окончания замеров с фильтрацией
    MWOS_PARAM(39, filterFinish, PARAM_BITS1, PARAM_TYPE_OPTIONS + PARAM_TYPE_READONLY, MWOS_STORAGE_NO, sensorsCount);
    // дополнительные настройки фильтров (общие для всех фильтров этого модуля)
    MWOS_PARAM(40, filterOptions, PARAM_FLOAT32, PARAM_TYPE_OPTIONS, MWOS_STORAGE_NVS, FilterOptionsCount);

    MWOSSensorFilter() : MWOSSensorAnalog<sensorsCount>() {
        for (MWOS_PARAM_INDEX_UINT i = 0; i < sensorsCount; ++i) {
            _coef[i] = 1;
            _subCoef[i] = 0;
            _filter[i] = EFM_None;
            sensorFilter[i] = NULL;
        }
        for (int n = 0; n < FilterOptionsCount; n++) {
            _filterOptions[n] = 0;
        }
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация (загружаем все настройки при старте)
            case EVENT_INIT: {
                for (int n = 0; n < FilterOptionsCount; n++) {
                    _filterOptions[n] = MWOSModule::loadValueDouble(_filterOptions[n], p_filterOptions, n);
                }
                for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; index++) {
                    ExtendedFilterMode sensorTypeOld = _filter[index];
                    _filter[index] = (ExtendedFilterMode) MWOSModule::loadValueInt((uint8_t) _filter[index], p_filter, index);
                    _coef[index] = MWOSModule::loadValueDouble(_coef[index], p_coef, index);
                    _subCoef[index] = MWOSModule::loadValueDouble(_subCoef[index], p_subCoef, index);

                    if (sensorTypeOld != _filter[index] && sensorFilter[index] != NULL) {
                        delete sensorFilter[index];
                        sensorFilter[index] = NULL;
                    }

                    if (_filter[index] != EFM_None && sensorFilter[index] == NULL) {
                        switch (_filter[index]) {
                            case EFM_Filter1: sensorFilter[index] = new SensorFilter1(); break;
                            case EFM_Filter2: sensorFilter[index] = new SensorFilter2(); break;
                            case EFM_Filter3: sensorFilter[index] = new SensorFilter3(); break;
                            case EFM_Filter4: sensorFilter[index] = new SensorFilter4(); break;
                            default: break;
                        }
                        if (sensorFilter[index] != NULL) sensorFilter[index]->filterOptions = _filterOptions;
                    }

                    if (sensorFilter[index] != NULL) {
                        sensorFilter[index]->Q = MWOSModule::loadValueDouble(sensorFilter[index]->Q, p_kalmanQ, index);
                        sensorFilter[index]->R = MWOSModule::loadValueDouble(sensorFilter[index]->R, p_kalmanR, index);
                        sensorFilter[index]->tau = MWOSModule::loadValueDouble(sensorFilter[index]->tau, p_kalmanTau, index);
                        sensorFilter[index]->reset();
                    }
                }
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_coef:
                        _coef[data.param_index] = data.toDouble();
                        MWOSModuleBase::SetParamChanged(&p_coef, data.param_index);
                        break;
                    case id_subCoef:
                        _subCoef[data.param_index] = data.toDouble();
                        MWOSModuleBase::SetParamChanged(&p_subCoef, data.param_index);
                        break;
                    case id_filter:
                        _filter[data.param_index] = (ExtendedFilterMode) data.toInt();
                        MWOSModuleBase::SetParamChanged(&p_filter, data.param_index);
                        break;
                    case id_kalmanQ:
                        if (sensorFilter[data.param_index] != NULL) {
                            sensorFilter[data.param_index]->Q = data.toDouble();
                        }
                        MWOSModuleBase::SetParamChanged(&p_kalmanQ, data.param_index);
                        break;
                    case id_kalmanR:
                        if (sensorFilter[data.param_index] != NULL) {
                            sensorFilter[data.param_index]->R = data.toDouble();
                        }
                        MWOSModuleBase::SetParamChanged(&p_kalmanR, data.param_index);
                        break;
                    case id_kalmanTau:
                        if (sensorFilter[data.param_index] != NULL) {
                            sensorFilter[data.param_index]->tau = data.toDouble();
                        }
                        MWOSModuleBase::SetParamChanged(&p_kalmanTau, data.param_index);
                        break;
                    case id_filterOptions:
                        _filterOptions[data.param_index] = data.toDouble();
                        MWOSModuleBase::SetParamChanged(&p_filterOptions, data.param_index);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка
            case EVENT_UPDATE: {
                // Тиковая обработка выполняется в базовом классе
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_result:
                        data.setValueDouble(getFloatValue(data.param_index));
                        break;
                    case id_coef:
                        data.setValueDouble((double) _coef[data.param_index]);
                        break;
                    case id_subCoef:
                        data.setValueDouble((double) _subCoef[data.param_index]);
                        break;
                    case id_predictedEKF:
                        data.setValueDouble(getFilteredValue(data.param_index));
                        break;
                    case id_filter:
                        data.setValueInt((int64_t) _filter[data.param_index]);
                        break;
                    case id_kalmanQ:
                        if (sensorFilter[data.param_index] != NULL) {
                            data.setValueDouble(sensorFilter[data.param_index]->Q);
                        } else {
                            data.setValueDouble(0);
                        }
                        break;
                    case id_kalmanR:
                        if (sensorFilter[data.param_index] != NULL) {
                            data.setValueDouble(sensorFilter[data.param_index]->R);
                        } else {
                            data.setValueDouble(0);
                        }
                        break;
                    case id_kalmanTau:
                        if (sensorFilter[data.param_index] != NULL) {
                            data.setValueDouble((double) sensorFilter[data.param_index]->tau);
                        } else {
                            data.setValueDouble(0);
                        }
                        break;
                    case id_filterFinish:
                        if (sensorFilter[data.param_index] != NULL) {
                            data.setValueInt(sensorFilter[data.param_index]->needFinish ? 1 : 0);
                        } else {
                            data.setValueInt(0);
                        }
                        break;
                    case id_filterOptions:
                        data.setValueDouble((double) _filterOptions[data.param_index]);
                        break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSSensorAnalog<sensorsCount>::onEvent(modeEvent, data);
    }

    virtual bool readBoolValue(uint16_t arrayIndex) override {
        // вызовем стандартный обработчик, чтобы считало новое значение
        bool res = MWOSSensorAnalog<sensorsCount>::readBoolValue(arrayIndex);

        // для EKF учтем полученное значение
        if (sensorFilter[arrayIndex] != NULL) {
            double measurement = getFloatValue(arrayIndex);
            sensorFilter[arrayIndex]->update(measurement, millis());
        }

        return res;
    }

    /**
    * Получить значение с учетом коэффициентов
    * @param arrayIndex Индекс датчика
    * @return Значение с коэффициентами
    */
    double getFloatValue(int16_t arrayIndex) {
        if (_coef[arrayIndex] == 0) _coef[arrayIndex] = 1;
        double value = ((double) MWOSSensorAnalog<sensorsCount>::_value[arrayIndex] - (double) _subCoef[arrayIndex]) * (double) _coef[arrayIndex];
        return value;
    }

    /**
    * Выдать значение с фильтром (если он задан)
    * @param arrayIndex Индекс датчика
    * @return Значение с фильтром
    */
    double getFilteredValue(int16_t arrayIndex) {
        if (sensorFilter[arrayIndex] != NULL) {
            if (MWOSSensorBase<sensorsCount>::isPaused()) return sensorFilter[arrayIndex]->getFinishResult();
            return sensorFilter[arrayIndex]->getCorrected();
        }
        return getFloatValue(arrayIndex);
    }
};

#endif