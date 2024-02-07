#ifndef MWOS3_MWOSMODULEPARAMS_H
#define MWOS3_MWOSMODULEPARAMS_H
/***
 * Базовый класс для всех модулей контроллера
 * Модули должны наследовать этот класс
 * в конструктор необходимо добавить все параметры модуля такой строкой:
 * MWOS_PARAM(Класс,Название,тип_результата,место_хранения)
 *
 * Пример:
    MWOSController() : MWOSModule() {
        // описание параметров
        MWOS_PARAM(MWOS_controllerID,cid,mwos_param_uint32,mwos_param_storage_eeprom);
        MWOS_PARAM(MWOS_sendTimeoutDSec,sendTimeoutDSec,mwos_param_uint16,mwos_param_storage_eeprom);
    }
 *
 */

#include "MWOSModuleBase.h"
#include "MWOSParam.h"
#include "MWOS3.h"

extern MWOS3 mwos;

/***
 * Ссылка на массив и его длина, упакованное в значение INT64
 */
struct PackValueArrayPrt {
    union {
        uint64_t value=0; // общий байт для всех битов настройки
        struct {
            uint32_t addr;
            uint32_t size;
        };
    };
};


class MWOSModule: public MWOSModuleBase {
public:

    MWOSModule(char * unit_name, uint16_t unit_id=0) : MWOSModuleBase(unit_name,unit_id) {
        if (mwos.AddChild(this)) mwos.modulesCount++;
    }

    /***
     * Сохраняет значение параметра в хранилище (если хранилище задано)
     * @param value Новое значение
     * @param param
     * @param arrayIndex
     */
    void saveValue(int64_t value, MWOSParam * param, int16_t arrayIndex=0) {
        if (param->IsLong()) {
            PackValueArrayPrt v;
            v.value=value;
            value=((uint8_t *) v.addr)[arrayIndex];
        }
        mwos.saveValue(value,this,param,arrayIndex);
    }

    /***
     * Сохраняет значение параметра в хранилище (если хранилище задано)
     * @param value Новое значение
     * @param paramId
     * @param arrayIndex
     */
    void saveValue(int64_t value, uint16_t paramId, int16_t arrayIndex=0) {
        saveValue(value,getParam(paramId),arrayIndex);
    }

    /**
     * Прочитать значение параметра из хранилища
     * @param param
     * @param arrayIndex
     * @return
     */
    int64_t loadValue(int64_t defValue, MWOSParam * param, int16_t arrayIndex=0) {
        int64_t res=mwos.loadValue(defValue,this,param,arrayIndex);
        if (param->IsGroup(mwos_param_option) && res!=defValue) { // это настройки и не значение по умолчанию
            switch ((uint8_t) param->storage) { // пометим хранилище модуля актуальным
                case 0:
                    if (!storage0Init) {
                        MW_LOG_MODULE(this); MW_LOG_LN(F("storage inited: 0"));
                        storage0Init=true;
                    }
                    break;
                case 1: storage1Init=true; break;
                case 2: storage2Init=true; break;
                case 3: storage3Init=true; break;
            }
        }
        return res;
    }

    /**
     * Упаковать байтовый массив в значение int64, пригодное для сохранения и отправки
     * @param buffer   байтовый массив
     * @param size   размер
     * @return  Упакованное значение
     */
    int64_t packByteArrayToValue(const uint8_t * buffer, size_t size) {
        PackValueArrayPrt v;
        v.addr=(uint32_t) buffer;
        v.size=size;
        return v.value;
    }

    /**
     * Упаковать строку в значение int64, пригодное для сохранения и отправки
     * @param str   Строка
     * @return  Упакованное значение
     */
    size_t packStringToValue(const char * str) {
        return packByteArrayToValue((uint8_t *) str, strlen(str));
    }

    /**
     * Упаковать строку в значение int64, пригодное для сохранения и отправки
     * @param str   Строка
     * @return  Упакованное значение
     */
    int64_t packStringToValue(const String &str) {
        return packByteArrayToValue((uint8_t *) str.c_str(), str.length());
    }

    /**
      * Прочитать значение параметра из хранилища
      * @param paramId
      * @param arrayIndex
      * @return
      */
    int64_t loadValue(int64_t defValue, int16_t paramId, int16_t arrayIndex=0) {
        return loadValue(defValue,getParam(paramId),arrayIndex);
    }

    /**
     * Вызывается при запросе значения параметра
     * @param paramNum  Номер параметра
     * @param arrayIndex Номер индекса в массиве значений параметра (если это массив)
     * @return  Значение
     */
    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        return loadValue(0,param,arrayIndex);
    }

    /***
     * Вызывается при изменении параметра
     * @param value     Значение для изменения
     * @param param     параметр
     * @param arrayIndex Номер индекса в массиве значений параметра (если это массив)
     */
    virtual void setValue(int64_t value, MWOSParam * param, int16_t arrayIndex) {
        SetParamChanged(param,arrayIndex, true);
        if (!param->IsGroup(mwos_param_readonly)) { // параметр не имеет флага readonly
            if (param->IsGroup(mwos_param_pin) && value>=0) { // для пинов, проверим что этот пин не занят
                if (mwos.FindByPin(value)!=NULL) value=-2; // этот пин уже занят - запишем (-2 - ошибка)
            }
            saveValue(value, param, arrayIndex); // сохраним в хранилище
        }
    }

    /***
     * Событие изменения значения для записи в журнал
     * @param value
     * @param param
     * @param arrayIndex
     */
    void toLog(int64_t value, MWOSParam * param, int16_t arrayIndex=0) {
        mwos.toLog(value,this,param,arrayIndex);
    }


};


#endif //MWOS3_MWOSMODULEPARAMS_H
