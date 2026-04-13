#ifndef MWOS3_MWOSLOG_H
#define MWOS3_MWOSLOG_H

#include "../../core/MWStreamLog.h"

#ifndef MWOS_LOG_RAM
#define MWOS_LOG_RAM 4096
#endif

/***
 * Модуль журнала
 * сохраняет события в журнал
 *
 *
    Формат журнала:
    В журнал сохраняются только события от параметров.
    Сохраняемое значение параметра не может быть массивом, но может быть строкой.
    Журнал автоматически отправляется на сервер, когда есть связь.
    Но данные в журнале удаляются только по команде от сервера.
    Сервер может повторно запросить данные из журнала.

    Взаимодействие журнала и сервера:
    При наличии связи, модуль net0 начинает передавать данные журнала.
    От начала журнала и сколько записей журнала влезет в пакет связи.
    Журнал при этом выставляет флаг "busy", запрещая модулю net0 дальнейшую передачу данных.
    Сервер в течении таймаута (10 сек) сообщает журналу, что можно удалить запипи первые N записей от id1 до id2
    (где N = сколько записей из журнала получил сервер).
    Если сервер удалил полученные записи из журнала, или вышел таймаут (10 сек), то флаг busy снимается.
    После этого модуль снова начинает передавать данные из начала журнала.
    Если сервер получает отметки с одинаковым id и временем, то это считается дубликатом и игнорируется
    id записи - просто число, которое увеличивается на 1 при каждой записи а журнал.


    Журнал состоит из фреймов. Внутри каждого фрейма может быть произвольное количество записей.
    Новый фрейм генерируется в следующих случаях:
    1. После включения или перезагрузки контроллера
    2. При изменении времени контроллера
    3. При достижении 65534 записей внутри фрейма (при переходе id=записи к 0)
    4. При неактивности событий более 65 сек
    5. После переполнения буфера журнала (в случае удаления неотправленных записей журнала)

    Формат фрейма (10 байт):
    2b: 10 (размер фрейма всегда 12 байт)
    2b: 0 (id записи < 10 - это признак фрейма) Типы фреймов: 0-полный (1-9 = резерв)
    8b - внутреннее время контроллера lunuxtime (в микросекундах)

    Формат записи события от 12-14 байт + данные (полный)
    2b: {размер данных} общий размер данных события (без учета этих 2х байт)
    4b: {id записи} - id предыдущей записи +1
    2b: {мСек} (сек/1000) от последней записи, если прошло более 60 сек, то начинаем новый фрейм
    2b: {id модуля}
    2b: {id параметра}
    2b: {index параметра} (только для нормальных параметров, для строк и байтовых массивов - индекс не передается)
    1b-255b: {данные} данные, до заданного размера

 */
class MWOSLog : public MWOSModule {
public:

    MWOSTime * time;
    MWStreamLog * stream=NULL;

    /**
     * Id последней непрочитанной записи
     */
    MWOS_PARAM(0, lastId, mwos_param_uint32, mwos_param_readonly, MWOS_STORAGE_RTC, 1);

    /**
     * Id первой не прочитанной записи (за последней прочитанной)
     */
    MWOS_PARAM(1, firstId, mwos_param_uint32, mwos_param_control, MWOS_STORAGE_RTC, 1);

    /**
     * Параметр для получения данных журнала
     */
    MWOS_PARAM(2, log, mwos_param_byte_array, mwos_param_readonly, MWOS_STORAGE_NO, 2);



    MWOSLog() : MWOSModule((char *) F("log")) {
        moduleType=ModuleType::MODULE_LOG;
        AddParam(&p_lastId);
        AddParam(&p_firstId);
        AddParam(&p_log);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT) { // инициализация ос
            if (stream == NULL) stream = new MWStreamLog(MWOS_LOG_RAM);
            time = MWOSTime::getTimeModule();
        }
    }

    /***
     * Вызывается при получении нового значения от сервера.
     * А так же при получении нового значения от модулей для журнала.
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->cmd==MWOSProtocolCommand::mwos_server_cmd_param_event) addEvent(receiverDat->module_id,receiverDat->param_id,receiverDat->array_index); // это событие для записи в журнал от другого модуля
        else
        if (receiverDat->param_id==1) delEvents(receiverDat->GetValueUInt32()); // команда удаления начала журнала
        // else MWOSModule::onReceiveValue(receiverDat);
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==0) return lastEventId();
        if (param->id==1) return firstEventId();
        if (param->id==2) return stream->getBlockByte(arrayIndex);
        return MWOSModule::getValue(param, arrayIndex);
    }

protected:
    uint32_t _record_id=0;
    uint64_t frameTime=0; // время записи фрейма

    uint16_t addFrame() {
        size_t size=0;
        size+=stream->write((uint16_t) 10); // размер фрейма
        if (mwos.id16bit) size+=stream->write((uint16_t) 0); // id записи
        else size+=stream->write((uint16_t) 1); // id записи (сокращенный формат)
        frameTime=time->getTimeMS();
        size+=stream->write((uint64_t) frameTime); // 10 запишем время [mSec]
        return size;
    }

    /**
     * Добавить событие от модуля в журнал.
     * @param module_id
     * @param param_id
     * @param array_index
     */
    uint16_t addEvent(uint16_t module_id, uint16_t param_id, uint16_t array_index) {
        MWOSModule * module=(MWOSModule *) mwos.FindChildById(module_id);
        MWOSParam * param=(MWOSParam *) module->FindChildById(param_id);
        size_t size=0;
        // размер блока
        uint16_t elementSize;
        uint16_t blockSize;
        if (param->IsLong()) {
            elementSize=param->arrayCount();
            blockSize=elementSize+12; // 2 полный размер блока
        } else {
            elementSize= param->byteSize(false);
            blockSize=elementSize+14; // 2 полный размер блока
        }
        // номер записи
        _record_id++;
        if (_record_id<10) {
            _record_id=10;
            frameTime=0;
        }
        //  относительное время
        uint64_t leftTime=0;
        if (frameTime==0) {
            size+=addFrame(); // добавим фрейм, если нужно
        } else {
            uint64_t nowTime=time->getTimeMS();
            uint64_t leftTime=nowTime-frameTime; // время с момента последней записи
            if (leftTime>65350) {
                size+=addFrame(); // добавим фрейм, если нужно
                leftTime=0;
            } else
                frameTime=nowTime;
        }
        // запишем блок
        size+=stream->beginBlock((uint16_t) blockSize); // 2 полный размер блока
        size+=stream->write((uint32_t) _record_id); // 6 id записи
        size+=stream->write((uint16_t) leftTime); // 8 запишем время [mSec]
        size+=stream->write((uint16_t) module_id); // 10
        size+=stream->write((uint16_t) param_id); // 12
        if (param->IsLong()) {
            for (MWOS_PARAM_INDEX_UINT i = 0; i < elementSize; i++) { // длинные параметры (string и byteArray) извлекаем побайтно
                uint8_t b= module->getValue(param, i);
                size+=stream->write((uint8_t) b);
            }
        } else {
            size+=stream->write((uint16_t) array_index); // 14
            int64_t value=module->getValue(param,array_index);
            uint8_t * block=(uint8_t *) &value;
            for (uint16_t i = 0; i < elementSize; ++i) {
                size+=stream->write((uint8_t) block[i]);
            }
        }
        p_log.arrayLength=(int16_t) stream->getBlockSize();
        SetParamChanged(&p_lastId);
        SetParamChanged(&p_log);
        return size;
    }

    /**
     * Получить id записи первого события в буфере
     * @return 0-нет или id записи
     */
    uint32_t firstEventId() {
        if (stream->count<1) return 0;
        return stream->GetValueUInt32(2);
    }

    /**
     * Получить id записи последнего события
     * @return id записи
     */
    uint32_t lastEventId() {
        return _record_id;
    }

    /**
     * Удалить первые события из списка (до события c заданным id записи)
     * @param event_id id записи события, до которого надо удалять первые события из журнала
     * @return  Количество реально удаленных событий
     */
    size_t delEvents(uint32_t event_id) {
        size_t res=0;
        while (firstEventId()<event_id && stream->clearFirstBlock()) {
            res++;
        }
        if (res>0) {
            p_log.arrayLength=(int16_t) stream->getBlockSize();
            SetParamChanged(&p_firstId);
            SetParamChanged(&p_log);
        }
        return res;
    }


};


#endif //MWOS3_MWOSLOG_H
