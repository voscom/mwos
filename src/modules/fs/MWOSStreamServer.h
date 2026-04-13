#ifndef MWOS35_MWOSSTREAMSERVER_H
#define MWOS35_MWOSSTREAMSERVER_H

#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"
#include "core/adlib/LibCRC.h"

#define MWOS_STREAM_BLOCK_SIZE 1024

#ifndef MWOS_STREAM_RECEIVE_TIMEOUT_PROGRESS_DSEC
/**
* Таймаут, через который отправлять на сервер прогресс загрузки файла (сек/10)
*/
#define MWOS_STREAM_RECEIVE_TIMEOUT_PROGRESS_DSEC 10
#endif

#ifndef MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK_DSEC
/**
* Таймаут, через который связь считается разорванной при загрузке файла (сек/10)
*/
#define MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK_DSEC 100
#endif
// переведем в 1/15 сек
const uint16_t MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK = MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK_DSEC*15/10;

#ifndef MWOS_STREAM_BLOCK_SIZE
/**
* Размер блока для отправки потока на сервер [половина буффера отправки]
*/
#define MWOS_STREAM_BLOCK_SIZE MWOS_SEND_BUFFER_SIZE / 2
#endif

const char MWOSStreamReceiveName[] PROGMEM = {"file"};

/**
* Модуль получения потоковых данных с произвольным доступом, заданного размера (как правило - файл) (Версия MWOS3.5)
*
* Команды:
*   start:0=0 - остановить прием
*   start:0=2 - начать передачу (c параметра file)
*   size:0=размер - начать прием данных, заданного размера. После этой команды должны идти потоковые данные на параметр file
*   progress:0={смещение} - установить смещение записи внутри потока
*
* Формат потока: (отправлять на file:0, приходит через EVENT_SET_VALUE)
*   (4 байт) - общий размер данных
*   событие: onStreamReceiveStart
*       далее идут данные пакетами:
*           (4 байт) - смещение записи (начала блока) внутри данных
*           (размер блока) - пакет данных не более размера буффера приема - 100
*               событие: onStreamReceiveNextByte для каждого байта пакета
*       пакеты повторяются, пока не будет передан весь объем данных
*   (8 байт) - crc64 всех данных обновления (без учета размера)
*   событие: onStreamReceiveStart
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - инициализация модуля.
*   EVENT_UPDATE - тиковая обработка (таймауты потока).
*   EVENT_SET_VALUE - обработка команд установки значений.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSStreamServer : public MWOSModule {
public:
    enum MWOS_STREAM_OPER : int8_t {
        mwos_stream_oper_none = 0,
        mwos_stream_oper_receive = 1,
        mwos_stream_oper_send = 3,
        mwos_stream_oper_copy_to = 4,
        mwos_stream_oper_copy_from = 5,
    };

    enum MWOS_STREAM_ERROR : int8_t {
        mwos_stream_no_error = 0,
        mwos_stream_error_block = 1,
        mwos_stream_error_md5 = 2,
        mwos_stream_error_space = 3,
        mwos_stream_error_timeout = 4,
        mwos_stream_error_seek = 5,
        mwos_stream_error_noname = 6,
        mwos_stream_error_open = 7,
        mwos_stream_error_size = 8,
        mwos_stream_error_write = 9,
    };

    enum MWOS_STREAM_CMD : int8_t {
        mwos_stream_cmd_skip = 0,
        mwos_stream_cmd_start_receive = 1,
        mwos_stream_cmd_set_seek = 2,
        mwos_stream_cmd_stop_receive = 3,
        mwos_stream_cmd_start_send = 4,
        mwos_stream_cmd_send_next_block = 5,
        mwos_stream_cmd_stop_send = 6,
        mwos_stream_cmd_error = 7,
        mwos_stream_cmd_copy_to = 8,
    };

    // --- Локальные переменные ---

    #pragma pack(push,1)
    MWOS_STREAM_OPER _oper = mwos_stream_oper_none; // команда обновления
    uint8_t _complete = 0; // команда обновления
    MWOS_STREAM_ERROR _error = mwos_stream_no_error;
    MWTimeout<uint16_t,15,true> timeoutReceiveStream;
    uint32_t _progress = 0;
    uint32_t _size = 0;
    uint32_t _seek = 0;
    #pragma pack(pop)

    MW_CRC64 crc64;

    // --- Объявление параметров (автоматическая регистрация) ---

    // команда потока
    MWOS_PARAM(0, start, PARAM_INT8, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // прогресс выполнения
    MWOS_PARAM(1, progress, PARAM_UINT32, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // размер потока
    MWOS_PARAM(2, size, PARAM_UINT32, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // код ошибки
    MWOS_PARAM(3, error, PARAM_INT8, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // на этот параметр надо отправлять файл
    MWOS_PARAM(5, file, PARAM_UINT8, PARAM_TYPE_READONLY + PARAM_TYPE_FILE, MWOS_STORAGE_NO, MWOS_STREAM_BLOCK_SIZE);

    MWOSStreamServer(char * unit_name = (char *) &MWOSStreamReceiveName) : MWOSModule(unit_name) {
        moduleType = MODULE_FILE;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация
            case EVENT_INIT: {
                // Инициализация при старте
            } break;

            // 2. Тиковая обработка (таймауты)
            case EVENT_UPDATE: {
                if (timeoutReceiveStream.isStarted() && timeoutReceiveStream.isTimeout()) {
                    stop(mwos_stream_error_timeout);
                }
            } break;

            // 3. Установка значения (замена onReceiveValue)
            case EVENT_SET_VALUE: {
                switch (data.param_id) {
                    case id_start: {
                        int8_t cmd = data.toInt();
                        if (cmd == 0) stop(mwos_stream_no_error);
                        if (cmd == 1) startReceive();
                        if (cmd == 2) startSend();
                        if (cmd == 3) onStreamEvent(mwos_stream_cmd_copy_to);
                    } break;
                    case id_progress: {
                        _seek = data.getUInt32(); // установим смещение
                        onStreamEvent(mwos_stream_cmd_set_seek);
                    } break;
                    case id_size: {
                        _size = data.getUInt32(); // установим размер
                        startReceive(); // начнем прием файла сразу после установки размера
                    } break;
                    case id_file: {
                        if (_oper == mwos_stream_oper_receive) {
                            if (_progress >= _size) { // прием закончен
                                if (data.size == 8) {
                                    uint64_t crc64now = crc64.getCRC();
                                    MW_LOG_MODULE(this); MW_LOG(F("crc64: ")); MW_LOG_BYTES((unsigned char *) &crc64now, 8);
                                    uint64_t recCRC = data.getUInt64();
                                    if (crc64now != recCRC) {
                                        MW_LOG(F(" != received crc64: ")); MW_LOG_BYTES((unsigned char *) &recCRC, 8); MW_LOG_LN();
                                        _complete = 0;
                                        stop(mwos_stream_error_md5);
                                        return;
                                    } else {
                                        MW_LOG_LN();
                                        _complete = 1;
                                        stop(mwos_stream_no_error);
                                        onStreamEvent(mwos_stream_cmd_stop_receive);
                                        return;
                                    }
                                }
                            } else {
                                crc64.addBuffer(data.getBytes(), data.size);
                                _progress += data.size;
                                onStreamReceiveBlock(data.getBytes(), data.size);
                                SetParamChanged(&p_progress);
                            }
                            timeoutReceiveStream.start(MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK);
                        }
                    } break;
                    default:
                        break;
                }
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_start:
                        data.setValueInt(_oper);
                        return;
                    case id_progress:
                        data.setValueInt(_progress);
                        return;
                    case id_size:
                        data.setValueInt(_size);
                        return;
                    case id_error:
                        data.setValueInt(_error);
                        return;
                    case id_file: {
                        if (_oper == mwos_stream_oper_send) {
                            uint8_t b = onStreamSendNextByte(data.param_index);
                            _progress++;
                            if (_progress >= _size) {
                                stop(mwos_stream_no_error);
                                _complete = 1;
                                MWOSModuleBase::SetParamChanged(&p_progress);
                                MWOSModuleBase::SetParamChanged(&p_start);
                                onStreamEvent(mwos_stream_cmd_stop_send);
                            } else if (data.param_index + 1 >= p_file.arrayLength) {
                                MWOSModuleBase::SetParamChanged(&p_file);
                                MWOSModuleBase::SetParamChanged(&p_progress);
                                onStreamEvent(mwos_stream_cmd_send_next_block);
                            }
                            data.setValueInt(b);
                            return;
                        } else {
                            if (data.param_index == 0) {
                                data.setValueInt(_complete);
                                return;
                            }
                        }
                        data.setValueInt(0);
                        return;
                    }
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSModule::onEvent(modeEvent, data);
    }

    /**
    * Вызывается как событие вначале и конце каждой операции (и при ошибках)
    */
    virtual void onStreamEvent(MWOS_STREAM_CMD cmd) {
        // Переопределяется в наследниках
    }

    /**
    * Прием блока данных
    */
    virtual void onStreamReceiveBlock(uint8_t * buff, uint16_t size) {
        // Переопределяется в наследниках
    }

    /**
    * Отправка следующего байта
    */
    virtual uint8_t onStreamSendNextByte(int16_t index) {
        return 0;
    }

    /**
    * Установить команду потока
    */
    void setCmd(MWOS_STREAM_OPER cmdCode) {
        _oper = cmdCode;
        MWOSModuleBase::SetParamChanged(&p_start);
        MW_LOG_MODULE(this); MW_LOG(F("setCmd: ")); MW_LOG_LN((uint8_t) cmdCode);
    }

    /**
    * Остановить поток
    */
    bool stop(MWOS_STREAM_ERROR errorCode) {
        setCmd(mwos_stream_oper_none);
        _error = errorCode;
        timeoutReceiveStream.stop();
        MWOSModuleBase::SetParamChanged(&p_error);
        MW_LOG_MODULE(this); MW_LOG(F("stop code: ")); MW_LOG_LN((uint8_t) _error);
        if (_error != mwos_stream_no_error) {
            onStreamEvent(mwos_stream_cmd_error);
            SetParamChanged(&p_error);
            return false;
        }
        return true;
    }

    /**
    * Начать прием данных
    */
    void startReceive() {
        setCmd(mwos_stream_oper_receive);
        p_file.arrayLength = 1;
        _progress = 0;
        _complete = 0;
        _error = mwos_stream_no_error;
        MWOSModuleBase::SetParamChanged(&p_error);
        crc64.start();
        MW_LOG_MODULE(this); MW_LOG_LN(F("startReceive!"));
        onStreamEvent(mwos_stream_cmd_start_receive);
        timeoutReceiveStream.start(MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK);
    }

    /**
    * Начать передачу данных
    */
    void startSend() {
        setCmd(mwos_stream_oper_send);
        p_file.arrayLength = MWOS_STREAM_BLOCK_SIZE;
        _progress = 0;
        _complete = 0;
        _error = mwos_stream_no_error;
        MWOSModuleBase::SetParamChanged(&p_error);
        MWOSModuleBase::SetParamChanged(&p_file);
        MW_LOG_MODULE(this); MW_LOG_LN(F("startSend!"));
        onStreamEvent(mwos_stream_cmd_start_send);
    }

    /**
    * Начать копирование данных на другой MWOSStreamServer
    */
    void startCopyTo() {
        setCmd(mwos_stream_oper_copy_to);
        p_file.arrayLength = MWOS_STREAM_BLOCK_SIZE;
        _progress = 0;
        _complete = 0;
        _error = mwos_stream_no_error;
        MWOSModuleBase::SetParamChanged(&p_error);
        MWOSModuleBase::SetParamChanged(&p_file);
        MW_LOG_MODULE(this); MW_LOG_LN(F("startCopyTo!"));
        onStreamEvent(mwos_stream_cmd_start_send);
    }

    /**
    * Начать прием данных от копирования с другого MWOSStreamServer
    */
    void startCopyFrom() {
        setCmd(mwos_stream_oper_copy_from);
        p_file.arrayLength = 1;
        _progress = 0;
        _complete = 0;
        _error = mwos_stream_no_error;
        MWOSModuleBase::SetParamChanged(&p_error);
        crc64.start();
        MW_LOG_MODULE(this); MW_LOG_LN(F("startCopyFrom!"));
        onStreamEvent(mwos_stream_cmd_start_receive);
        timeoutReceiveStream.start(MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK);
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif