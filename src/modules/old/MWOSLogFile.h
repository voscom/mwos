#ifndef MWOS3_MWOSLOGFILE_H
#define MWOS3_MWOSLOGFILE_H

#include "../../fs/MWFS.h"
#include "MWOSLog.h"

/**
 * Модуль журнала с записью в файл
 * сохраняет события в журнале. Может накапливать события, пока отсутствует связь с сервером
 * Передает события из журнала на сервер с подтверждением (гарантированная доставка)
 *
 * События сохраняются в файл "/ev.log" при следующих условиях:
 * 1. Пришло событие EVENT_POWER_OFF
 * 2. Буфер в ОЗУ переполнен (или размер данных в ОЗУ превысил размер блока файла)
 * События из буфера ОЗУ дописываются в файл "/ev.log", буфер ОЗУ очищается и запоминается указатель в файле на первое неотправленное событие.
 * В случае, когда все события из файла отправлены на сервер - файл удаляется.
 *
 * В новом файле id записи начинается сначала (от 10)
 *
 * Если Id последней прочитанной записи = 0, то сервер посылает этот Id.
 * При этом он ищется в файле и указатель чтения устанавливается на следующее за ним событие.
 *
 * 1. При включении контроллера открывает файл "/ev.log" на дополнение
 * 2. Все новые события дописываются в конец этого файла
 * 3. Читает номер первого события в файле для параметра firstId
 * 4. Читает номер последнего события в файле для параметра lastId
 * 5. Сервер устанавливает параметр firstId на последнюю прочитанную запись
 * 6. Следующее за firstId событие отправляется на сервер. И снова с п.5
 * 7. Когда отправлено последнее событие из файла - файл удаляется
 *
 * Поиск события по recordId происходит по 10 записей каждый тик
 * Если искомая запись ближе к firstId то ищет по нарастанию от начала,
 * если ближе к lastId то по убыванию от конца
 * Если записей между firstId и lastId очень много - можно искать запись посередине (метод поиска в индексированном массиве)
 */
class MWOSLogFile : public MWOSLog {
public:

    MWOSFS * _fs;
    File _fileRead; // указатель на читаемые с начала файла события (первое неотправленное событие)
//#pragma pack(push,1)
    uint16_t _eventSize=0; // размер текущего события
    uint8_t * _eventBuff=NULL; // буфер, содержащий текущее событие
    uint8_t _errorCode=0;
    uint32_t _lastId=0;
    uint32_t _firstId=0;
//#pragma pack(pop)


    MWOSLogFile() : MWOSLog() {
    }

    MWOSLogFile(MWOSFS * forFS) : MWOSLogFile() {
        _fs=forFS;
    }


    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_POWER_OFF) { // при выключении
            saveToFile();
        } else
        if (modeEvent==EVENT_INIT) { // инициализация ос
            if (_fs->fs->exists("/ev.log")) {
                _fileRead=_fs->fs->open("/ev.log", MWFILE_READ); // чтение очередного события для отправки на сервер
                if (_fileRead.size()>0) {
                    _fileRead.seek(_fileRead.size());
                    _lastId=skipPrevEvent();
                    _fileRead.seek(0);
                    _firstId=readNextEvent();
                }
            }
        }
        MWOSLog::onEvent(modeEvent);
    }

    /***
     * Вызывается при получении нового значения от сервера.
     * А так же при получении нового значения от модулей для журнала.
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        MWOSLog::onReceiveValue(receiverDat);
        if (stream->available()>=_fs->fs->page()) saveToFile();
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        return MWOSLog::getValue(param, arrayIndex);
    }

protected:

    void saveToFile() {
        if (stream->available()==0) return;
        File fileAdd=_fs->fs->open("/ev.log", MWFILE_APPEND); // запись в конец файла
        if (stream->offsetRead<stream->offsetWrite) {
            fileAdd.write(stream->bufferRAM + stream->offsetRead, stream->available());
        } else {
            fileAdd.write(stream->bufferRAM + stream->offsetRead, stream->total - stream->offsetRead);
            fileAdd.write(stream->bufferRAM, stream->offsetWrite);
        }
        fileAdd.flush();
        fileAdd.close();
        stream->clear();
    }

    /**
     * Пропустить очередное событие из файла и вернуть его recordId.
     * Указатель файла указывает на начало следующего события.
     * @return  0 или recordId
     */
    uint32_t skipNextEvent() {
        uint16_t eventSize=0;
        uint32_t recordId=0;
        size_t siz=_fileRead.readBytes((char *) &eventSize,2);
        if (siz==2 && eventSize>9) {
            siz=_fileRead.readBytes((char *) recordId,4);
            if (siz==4) {
                _fileRead.seek(_fileRead.peek()+eventSize-2); // указатель на следующее событие
                return recordId;
            } else {
                setError(11);
            }
        } else {
            setError(12);
        }
        _fileRead.seek(_fileRead.size()); // указатель в конец файла
        return 0;
    }

    /**
     * Пропустить предыдущее событие из файла и вернуть его recordId.
     * Указатель файла указывает в начало этого события.
     * @return  0 или recordId
     */
    uint32_t skipPrevEvent() {
        uint16_t eventSize=0;
        uint32_t recordId=0;
        _fileRead.seek(_fileRead.peek()-2); // указатель на размер предыдущего события
        size_t siz=_fileRead.readBytes((char *) &eventSize,2);
        if (siz==2 && eventSize>9) {
            int32_t pos=_fileRead.peek()-eventSize-2;
            if (pos>=2) {
                _fileRead.seek(pos); // указатель на Id предыдущего события
                siz=_fileRead.readBytes((char *) recordId,4);
                if (siz==4) {
                    _fileRead.seek(_fileRead.peek()-6); // указатель на начало предыдущего события
                    return recordId;
                } else {
                    setError(21);
                }
            } else {
                setError(22);
            }
        } else {
            setError(23);
        }
        _fileRead.seek(0); // указатель в начало файла
        return 0;
    }

    /**
     * Прочитать очередное событие из файла в _eventBuff, если нет, то _eventBuff=NULL
     * @return  0 или recordId
     */
    uint32_t readNextEvent() {
        ClearBuffer(); // освободим буфер
        size_t siz=_fileRead.readBytes((char *) &_eventSize,2);
        if (siz==2 && _eventSize>9) {
            _eventBuff=new uint8_t(_eventSize);
            siz=_fileRead.readBytes((char *) _eventBuff,_eventSize);
            if (siz==_eventSize) {
                uint16_t eventSize=0;
                if (_fileRead.readBytes((char *) &eventSize,2)==2) { // еще раз размер события записан в конце блока
                    if (eventSize==_eventSize) {  // длина считанного соответствует размеру события (указатель уже на следующем событии)
                        uint32_t res=0;
                        memcpy(&res, _eventBuff, 4); // скопируем recordId в res
                        return res;
                    } else {
                        setError(1);
                    }
                } else {
                    setError(2);
                }
            } else {
                setError(3);
            }
        } else {
            setError(4);
        }
        ClearBuffer(); // освободим буфер
        _fileRead.seek(_fileRead.size()); // указатель в конец файла
        return 0;
    }

    void setError(uint8_t errorCode) {
        if (_errorCode!=errorCode) {
            MW_LOG_MODULE(this); MW_LOG(F("Error: ")); MW_LOG_LN(errorCode);
            _errorCode=errorCode;
        }
    }

    /**
     * Освободить буфер событий _eventBuff, если он был
     */
    void ClearBuffer() {
        if (_eventBuff!=NULL) {
            delete[] _eventBuff; // освободим предыдущий буфер
        }
        _eventSize=0;
        _eventBuff=NULL;
    }

    String todayFilename() {
        uint64_t day=time->getTimeMS()/(360000*24); // сутки в linuxTime
        String fileName="/"+String((uint32_t) day);
        fileName+=".log";
        return fileName;
    }


};


#endif //MWOS3_MWOSLOGFILE_H
