#ifndef MWOS35_MWOSFILE_H
#define MWOS35_MWOSFILE_H

#include "fs/MWFS.h"
#include "MWOSStreamServer.h"

/**
* Модуль приема и отправки файла из файловой системы через модуль связи (Версия MWOS3.5)
*
* Команды терминала файловой системы:
*   //залить файл на контроллер с сервера из папки upload
*   25:file=$up;/test.zip
*   //скачать файл с контроллера на сервер в папку upload
*   25:file=$dw;/test.zip
* где 25 - id контроллера, file - модуль файлов для файловой системы
* Отправляет и получает файлы ТОЛЬКО из соответствующего подкаталога upload с сервера
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - инициализация модуля.
*   EVENT_UPDATE - тиковая обработка (копирование файлов).
*   EVENT_SET_VALUE - обработка команд установки значений.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSFile : public MWOSStreamServer {
public:
    // указатель на модуль файловой системы
    MWOSFS * _fs;
    // ссылка на другой приемник файла для команды копирования
    MWOSStreamServer * forCopy;
    // имя текущего файла
    String _filename;
    // буфер для данных
    uint8_t buff[MWOS_STREAM_BLOCK_SIZE];
    // файл
    File _file;

    // --- Объявление параметров (автоматическая регистрация) ---

    // имя текущего файла
    MWOS_PARAM(6, name, PARAM_STRING, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 255);
    // id модуля с файловой системой
    MWOS_PARAM(7, fsModule, MWOS_PARAM_INT_PTYPE, PARAM_TYPE_MODULE_ID + PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);
    // id модуля (наследник MWOSStreamServer), для операции копирования
    MWOS_PARAM(8, copyToModule, MWOS_PARAM_INT_PTYPE, PARAM_TYPE_MODULE_ID + PARAM_TYPE_OPTIONS, MWOS_STORAGE_EEPROM, 1);

    MWOSFile() : MWOSStreamServer((char *) F("file")) {
        moduleType = MODULE_FILE;
    }

    MWOSFile(MWOSFS * forFS) : MWOSFile() {
        _fs = forFS;
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Инициализация
            case EVENT_INIT: {
                // Файловая система
                _fs = (MWOSFS *) loadLinkModule(*_fs, MODULE_FS, p_fsModule, 0);
                // Модуль для копирования
                forCopy = (MWOSStreamServer *) loadLinkModule(*forCopy, MODULE_FILE, p_copyToModule, 0);
            } break;

            // 2. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                switch (data.param_id) {
                    case id_fsModule:
                        _fs = (MWOSFS *) loadLinkModule(*_fs, MODULE_FS, p_fsModule, 0);
                        MWOSModuleBase::SetParamChanged(&p_fsModule);
                        break;
                    case id_copyToModule:
                        forCopy = (MWOSStreamServer *) loadLinkModule(*forCopy, MODULE_FILE, p_copyToModule, 0);
                        MWOSModuleBase::SetParamChanged(&p_copyToModule);
                        break;
                    default:
                        break;
                }
            } break;

            // 3. Тиковая обработка (копирование файлов)
            case EVENT_UPDATE: {
                if (_oper == mwos_stream_oper_copy_to && forCopy != NULL) { // идет копирование
                    if (_progress >= _size) { // закончим копирование
                        onStreamEvent(mwos_stream_cmd_stop_send);
                        forCopy->onStreamEvent(mwos_stream_cmd_stop_receive);
                    } else { // копируем поблочно
                        onStreamEvent(mwos_stream_cmd_send_next_block);
                        _progress += p_file.arrayLength;
                        forCopy->onStreamReceiveBlock(buff, p_file.arrayLength);
                        forCopy->_progress = _progress;
                        timeoutReceiveStream.start(MWOS_STREAM_RECEIVE_TIMEOUT_BLOCK_DSEC);
                        if (forCopy->_error != 0) stop(mwos_stream_error_write);
                    }
                }
            } break;

            // 4. Установка значения (замена onReceiveValue)
            case EVENT_SET_VALUE: {
                if (data.param_id == id_name) {
                    _filename = data.toString();
                    _file = _fs->fs->open(_filename, MWFILE_READ);
                    if (_file) {
                        _size = _file.size();
                        _file.close();
                        p_name.arrayLength = _filename.length() + 1;
                    } else {
                        _size = 0;
                        p_name.arrayLength = 1;
                    }
                    MWOSModuleBase::SetParamChanged(&p_name);
                    MWOSModuleBase::SetParamChanged(&p_size);
                    MW_LOG_MODULE(this); MW_LOG(F("set filename: ")); MW_LOG_LN(_filename);
                }
            } break;

            // 5. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_name:
                        if (_filename.length() > (unsigned int) data.param_index) {
                            data.setValueInt(_filename[data.param_index]);
                        } else {
                            data.setValueInt(0);
                        }
                        return;
                }
            } break;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSStreamServer::onEvent(modeEvent, data);
    }

    /**
    * Обработка событий потока
    */
    virtual void onStreamEvent(MWOS_STREAM_CMD cmd) override {
        switch (cmd) {
            case mwos_stream_cmd_start_receive: {
                if (_filename != "") {
                    _file = _fs->fs->open(_filename, MWFILE_WRITE);
                    if (!_file) stop(mwos_stream_error_open);
                } else {
                    stop(mwos_stream_error_noname);
                }
            } break;
            case mwos_stream_cmd_stop_receive: {
                if (_file) _file.close();
            } break;
            case mwos_stream_cmd_start_send: {
                if (_filename != "") {
                    _file = _fs->fs->open(_filename, MWFILE_READ);
                    if (!_file) stop(mwos_stream_error_open);
                    else if (_oper == mwos_stream_oper_send) onStreamEvent(mwos_stream_cmd_send_next_block);
                } else {
                    stop(mwos_stream_error_noname);
                }
            } break;
            case mwos_stream_cmd_send_next_block: {
                p_file.arrayLength = _file.read((uint8_t *) &buff, MWOS_STREAM_BLOCK_SIZE);
            } break;
            case mwos_stream_cmd_stop_send: {
                if (_file) _file.close();
            } break;
            case mwos_stream_cmd_error: {
                if (_file) _file.close();
                if (_oper == mwos_stream_oper_receive) _fs->fs->remove(_filename);
            } break;
            case mwos_stream_cmd_copy_to: {
                if (forCopy != NULL) {
                    forCopy->startCopyFrom();
                    if (forCopy->_error == 0) startCopyTo();
                }
            } break;
            default: {}
        }
    }

    /**
    * Прием блока данных
    */
    virtual void onStreamReceiveBlock(uint8_t * buff, uint16_t size) override {
        if (!_file) {
            stop(mwos_stream_error_write);
            return;
        }
        _file.write(buff, size);
#ifndef MWOS_SD_CARD
        _file.flush(); // сохраняем каждый блок
#endif
        MW_LOG_MODULE(this); MW_LOG(F("file write: ")); MW_LOG_LN(size);
    }

    /**
    * Отправка следующего байта
    */
    virtual uint8_t onStreamSendNextByte(int16_t index) override {
        return buff[index];
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif