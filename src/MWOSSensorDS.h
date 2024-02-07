#ifndef MWOS3_MWOSSENSORDS_H
#define MWOS3_MWOSSENSORDS_H
/***
 * Несколько датчиков температуры DS18x20 на одном порту
 * Если список датчиков пуст, то производит поиск всех датчиков на линии DS. Но не более {sensorsCount}
 * В дальнейшем, можно принудительно запускать поик новых датчиков удаленной командой (с клиента)
 * Поиск будет происходить только для незаполненных ранее (или удаленных) датчиков.
 * Неиспользуемые датчики тоже можно удалить только удаленной командой (с клиента). Например: "133:ds0:address:2=-1"
 * Неиспользуемые или удаленные датчики имеют адрес 0 (параметр 18)
 *
 * Команды: TODO: надо сделать!!!
 * 133:ds0:address:2=-1  // удаление DS-датчика, запомненного в 133:ds0:address:2
 * 133:ds0:address=0 // запустить поиск новых датчиков на линии DS
 *
 */
#include "core/MWOSSensorAnalog.h"
#include <OneWire.h>

// показания вне этого диапазона - ошибка
#define minValue	-30*16
#define maxValue	80*16

template<MWOS_PARAM_INDEX_UINT sensorsCount>
class MWOSSensorDS : public MWOSSensorAnalog<sensorsCount> {
public:
    MWOS_PIN_INT _pin=-1;
    MWOS_PIN_INT _pinPower=-1;
    int8_t _errorCode=0;
    int8_t initedStep; // текущий датчик DS
    OneWire * ds=NULL;
    int64_t _address[sensorsCount]; // DS-адреса
    int16_t lastV[sensorsCount]; // последние значения замеров

    //************************ описание параметров ***********************/
    // порт для датчиков DS
    MWOS_PARAM(1, pin, MWOS_PIN_INT_PTYPE, mwos_param_pin, mwos_param_storage_eeprom, 1);
    // порт включения питания для датчиков DS
    MWOS_PARAM(18, power, MWOS_PIN_INT_PTYPE, mwos_param_pin, mwos_param_storage_eeprom, 1);
    // адреса DS для каждого обнаруженного датчика
    MWOS_PARAM(19, address, mwos_param_int64, mwos_param_readonly+mwos_param_option, mwos_param_storage_eeprom, sensorsCount);

    /***
     * Создать датчики
     */
    MWOSSensorDS() : MWOSSensorAnalog<sensorsCount>() {
        MWOSModuleBase::setName((char *) F("ds"));
        MWOSModuleBase::AddParam(&p_pin);
        MWOSModuleBase::AddParam(&p_power);
        MWOSModuleBase::AddParam(&p_address);
    }

    /***
     * Создать датчики
     * @param port порт датчиков DS
     */
    MWOSSensorDS(MWOS_PIN_INT pin) : MWOSSensorDS<sensorsCount>() {
        _pin=pin;
    }

    virtual void onInit() {
        for (uint16_t i=0; i<sensorsCount; i++) _address[i]=0;
        initedStep=-1;
        MWOSSensorAnalog<sensorsCount>::onInit();
        _pin=MWOSSensorAnalog<sensorsCount>::loadValue(_pin, &p_pin, 0);
        _pinPower=MWOSSensorAnalog<sensorsCount>::loadValue(_pinPower, &p_power, 0);
        if (_pin<0) return;
        mwos.pin(_pin)->mode(false, MWOSSensorAnalog<sensorsCount>::_sensor_pull);
        if (ds==NULL) ds=new OneWire(_pin);
        else ds->begin(_pin);
        if (initedStep==-1) updateAllDS(); // пустой список адресов - вызывает поиск датчиков DS
    }

    virtual void initSensor(int16_t index, bool pullOn) {
        MWOSSensorAnalog<sensorsCount>::initSensor(index, pullOn);
        _address[index]=MWOSSensorAnalog<sensorsCount>::loadValue(_address[index], &p_address, index);
        if (_address[index]!=0) initedStep=0;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==1) return _pin;
        if (param->id==18) return _pinPower;
        if (param->id==19) return _address[arrayIndex];
        return MWOSSensorAnalog<sensorsCount>::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual void setValue(int64_t v, MWOSParam * param, int16_t arrayIndex) {
        if (param->id==19) { // попытка записи в address - это команда
            if (v==0) MWOSSensorAnalog<sensorsCount>::saveValue(v,param,arrayIndex); // сохраним в EEPROM 0 вместо адреса DS  (сбросим адрес)
            if (v==-1) updateAllDS(); // команда для этого параметра - вызывает поиск датчиков DS
        }
        MWOSSensorAnalog<sensorsCount>::setValue(v,param,arrayIndex);
    }


    /**
     * Опросить аналоговый датчик для получения новых показаний
     *
     */
    virtual int32_t readAnalogValue(int16_t arrayIndex) {
        if (!isSetAddress(arrayIndex) || _pin<0) return 0;
        bool measuring=MWOSSensorAnalog<sensorsCount>::_value_bin[arrayIndex].bit_req; // третий бит значения - был запрос на измерение температуры
        if (measuring) { // ранее было задано измерение температуры
            MWOSSensorAnalog<sensorsCount>::_value_bin[arrayIndex].bit_req=0; // сбросим измерение
            lastV[arrayIndex]=readDSValue(arrayIndex);
        } else {
            MWOSSensorAnalog<sensorsCount>::_value_bin[arrayIndex].bit_req=1; // задали измерение
            selectDS(arrayIndex);
        }
        return lastV[arrayIndex];
    }

    /***
     * Ищет все датчики DS на линии и добавляет их в настройки
     * Если места в настройках не хватает - генерирует ошибку 3
     */
    void updateAllDS() {
        if (_pin<0) return;
        int64_t addr64=0;
        uint8_t * addr=(uint8_t *)addr64;
        while (ds->search(addr))  { // нашли очередное DS-устройство
            if (((addr[0] == 0x10) || (addr[0] == 0x28) || (addr[0] == 0x22)) && (OneWire::crc8(addr, 7) == addr[7])) {
                if (indexOfAddress(addr64)<0) { // такого адреса нет
                    int16_t emptyNum=indexOfAddress(0); // ближайший пустой адрес
                    if (emptyNum<0) {
                        setError(3); // все адреса заполнены!
                    } else {
                        _address[emptyNum]=addr64;
                        MWOSSensorAnalog<sensorsCount>::saveValue(addr64,&p_address,emptyNum);
                    }
                }
            }
        }
        ds->reset_search();
    }

    /**
     * Найти порядковый номер ардеса в списке адресов
     * @param addr64
     * @return  индекс адреса (-1, если не найдено)
     */
    int16_t indexOfAddress(int64_t addr64) {
        for (uint16_t i = 0; i < sensorsCount; ++i) {
            if (_address[i]==addr64) return i;
        }
        return -1;
    }

    bool isSetAddress(int16_t numDS) {
        return numDS<(int16_t) sensorsCount && _address[numDS]>0;
    }

    /**
     * Выбрать текущий DS на линии и включить замер температуры
     * @return Выбрано успешно
     */
    bool selectDS(int16_t numDS) {
        uint8_t * addDS=(uint8_t *) &_address[numDS];
        if (!isSetAddress(numDS)) return false;
        if (ds->reset()==0) {
            MW_LOG_LN(F("1wire reset error!"));
            return false;
        }
        ds->select(addDS);
        ds->write(0x44, 1);
        return true;
    }

    int16_t readDSValue(int16_t numDS) { // прочитать результат опроса термодатчика
        if (ds->reset()==0) {
            MW_LOG_LN(F("1wire p_reset error!"));
            setError(1);
            return INT16_MAX;
        }
        uint8_t * addDS=(uint8_t *) &_address[numDS];
        ds->select(addDS);
        ds->write(0xBE);         // Read Scratchpad
        uint8_t data[12];
        for (uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
            data[i] = ds->read();
        }
        // проверим сумму
        if (OneWire::crc8(data,8) != data[8]) {
            MW_LOG_LN(F("1wire crc error!"));
            setError(2);
            return INT16_MAX;
        }
        // Convert the data to actual temperature
        int16_t raw = (data[1] << 8) | data[0];
        if (addDS[0]==0x10) {
            raw = raw << 3; // 9 bit resolution default
            if (data[7] == 0x10) { // "count remain" gives full 12 bit resolution
                raw = (raw & 0xFFF0) + 12 - data[6];
            }
        } else {
            uint8_t cfg = (data[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
            //// default is 12 bit resolution, 750 ms conversion time
        }
        //celsius = ((float)raw) / 16.0f;
        setError(0); // нет ощибки
        return raw;
    }

    void setError(uint8_t errorCode) {
        if (_errorCode!=errorCode) {
            MWOSSensorAnalog<sensorsCount>::SetParamChangedByParamId(17,0,true);
            if (errorCode!=0) {
                MW_LOG_MODULE(this); MW_LOG(F("ERROR CODE: ")); MW_LOG_LN(errorCode);
            }
        }
        _errorCode=errorCode;
    }

    virtual int8_t getErrorCode() {
        return _errorCode;
    }


};


#endif //MWOS3_MWOSSENSORDS_H
