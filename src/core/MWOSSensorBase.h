#ifndef MWOS3_MWOSSENSORBASE_H
#define MWOS3_MWOSSENSORBASE_H
/***
 * Базовый модуль для бинарных датчиков
 * Только для наследования из других модулей датчиков!
 * Необходимо в потомках определить параметр портов:
 * MWOS_PARAM(1,sensor_pin, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom,sensorsCount);
 */

// минимальное время включения подтяжки на портах перед замером показаний
#ifndef MWOS_SENSOR_PULL_TIME_MS
#define MWOS_SENSOR_PULL_TIME_MS 50
#endif

template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorBase : public MWOSModule {
public:
#pragma pack(push,1)
    struct EverySensorBits {
        union {
            uint8_t _value = 0; // общий байт для всех битов настройки
            struct {
                uint8_t bin_value: 1; // текущее бинарное значение датчика
                uint8_t bit_changed: 1; // бинарное значение было изменено
                uint8_t bit_filter_now: 1; // включен бинарный фильтр
                uint8_t bit_req: 1; // включен запрос
                uint8_t analog_changed: 1; // бинарное значение было изменено
                uint8_t bit_reserve: 3;
            };
        };
    };

    EverySensorBits _value_bin[sensorsCount]; // бинарные показания (например: 0-вкл, 1-выкл)
    MWTimeout waitTimeout[sensorsCount];
    uint8_t _reserve:1;
    uint8_t waitPool:1;
    uint8_t _invert:1;      // инвертировать значение
    uint8_t _pull_off:1;    // выключать подтяжку порта датчика между опросами
    uint8_t _always_off:1;  //  всегда отключено (одновременно c sensor_invert - всегда включено)
    uint8_t _bin_to_log:1;  //  сохранять бинарные показания в журнал
    uint8_t _sensor_pull:2; //подтяжка (0-нет, 1-на 0, 2-на питание, 3 - открытый коллектор)
    uint8_t _bin_filter=0;  // фильтр переключения бинарного значения [сек/10] (кратковременные переключения, меньше этого значения - игнорируются)
    uint16_t _get_timeout=0; // Минимальное время [сек/10], между опросами показаний датчика
    MWTimeout lastRead;
#pragma pack(pop)
    //************************ описание параметров ***********************/

    // бинарные показания (например: 0-вкл, 1-выкл)
    MWOS_PARAM(0, valueBin, mwos_param_bits1, mwos_param_sensor + mwos_param_readonly, mwos_param_storage_no, sensorsCount);
    //MWOS_PARAM(1,pin, MWOS_PARAM_PIN_INT, mwos_param_pin, mwos_param_storage_eeprom,sensorsCount);  // порты
    // фильтр переключения бинарного значения [сек/10] (кратковременные переключения, меньше этого значения - игнорируются)
    MWOS_PARAM(2, binFilter, mwos_param_uint8, mwos_param_option, mwos_param_storage_eeprom, 1);
    // Минимальное время [сек/10], между опросами показаний датчика
    MWOS_PARAM(3, getTimeout, mwos_param_uint16, mwos_param_option, mwos_param_storage_eeprom, 1);
    // инвертировать значение
    MWOS_PARAM(4, invert, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    // выключать подтяжку порта датчика между опросами
    MWOS_PARAM(5, pullOff, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    //  всегда отключено (одновременно c sensor_invert - всегда включено)
    MWOS_PARAM(6, alwaysOff, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);
    //подтяжка (0-нет, 1-на 0, 2-на питание, 3 - открытый коллектор)
    MWOS_PARAM(7, pull, mwos_param_bits2, mwos_param_option, mwos_param_storage_eeprom, 1);
    //  сохранять бинарные показания в журнал
    MWOS_PARAM(8, binToLog, mwos_param_bits1, mwos_param_option, mwos_param_storage_eeprom, 1);

    MWOSSensorBase() : MWOSModule((char *) F("sensor")) {
        AddParam(&p_valueBin);
        AddParam(&p_binFilter);
        AddParam(&p_getTimeout);
        AddParam(&p_invert);
        AddParam(&p_pullOff);
        AddParam(&p_alwaysOff);
        AddParam(&p_pull);
        AddParam(&p_binToLog);
    }

    virtual void onInit() {
        _invert=loadValue(_invert, &p_invert);
        _pull_off=loadValue(_pull_off, &p_pullOff);
        _always_off=loadValue(_always_off, &p_alwaysOff);
        _sensor_pull=loadValue(_sensor_pull, &p_pull);
        _bin_to_log=loadValue(_bin_to_log, &p_binToLog);
        _bin_filter=loadValue(_bin_filter, &p_binFilter);
        _get_timeout=loadValue(_get_timeout, &p_getTimeout);
        for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; ++index) {
            initSensor(index,false);
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex= 0) {
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 0: {
                if (_always_off) _value_bin[arrayIndex].bin_value=0;
                if (_invert) return !_value_bin[arrayIndex].bin_value;
                return _value_bin[arrayIndex].bin_value;
            }
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    bool getValueBool(int16_t arrayIndex= 0) {
        return getValue(&p_valueBin,arrayIndex);
    }

    virtual void setValue(int64_t value, MWOSParam * param, int16_t arrayIndex= 0) {
        if (param==&p_valueBin) return;
        MWOSModule::setValue(value,param,arrayIndex);  // сохраним в хранилище
        onInit(); // обновим настройки
    }

    /***
     * Инициализация каждого датчика
     * @param index
     * @param pullOn
     */
    virtual void initSensor(int16_t index, bool pullOn) {
        MW_LOG_MODULE(this); MW_LOG(F("init sensor ")); MW_LOG(index); MW_LOG('='); MW_LOG_LN(_sensor_pull);
    }

    /***
     * Вызывается каждый тик операционной системы
     */
    virtual void onUpdate() {
        if (_always_off) return;
        if (lastRead.isTimeout()) {
            if (_pull_off) { // необходимо включить подтяжку перед измерением
                if (!waitPool) {
                    for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; ++index) initSensor(index, true);
                    lastRead.startMS(MWOS_SENSOR_PULL_TIME_MS);
                    waitPool=true;
                    return; // чтобы не ждать пока устаканится подтяжка - проверим сенсоры в следующий раз
                } else {
                    waitPool=false;
                }
            }
            for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; index++) {
                readSensor(index);
            }
            if (_pull_off) for (MWOS_PARAM_INDEX_UINT index = 0; index < sensorsCount; ++index) initSensor(index, false); // выключим подтяжку
            lastRead.start(_get_timeout);
        }
    }

    void readSensor(uint16_t index) {
        bool newValueBin=readBoolValue(index);
        if (newValueBin != _value_bin[index].bin_value) {
            if (_bin_filter > 0) {
                if (!_value_bin[index].bit_filter_now) { // надо подождать время, заданное в фильтре
                    _value_bin[index].bit_filter_now=1;
                    waitTimeout[index].start(_bin_filter);
                    return;
                } else {
                    if (!waitTimeout[index].isTimeout()) return;
                    _value_bin[index].bit_filter_now=0;
                }
            }
            _value_bin[index].bin_value = newValueBin;
            MW_LOG_MODULE(this); MW_LOG(F("sensor bin ")); MW_LOG(index); MW_LOG('='); MW_LOG_LN(newValueBin);
            if (_bin_to_log) {
                toLog(newValueBin, &p_valueBin, index);
            } else
                SetParamChanged(&p_valueBin, index, true);
        }
    }


    /***
     * Вызывается через таймаут для всех индексов
     * @param index
     * @return
     */
    virtual bool readBoolValue(uint16_t index) {
        return true;
    }




};


#endif //MWOS3_MWOSSENSORBASE_H
