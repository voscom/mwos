#ifndef MWOS35_MWOSFS_H
#define MWOS35_MWOSFS_H

#include "fs/MWFS.h"
#include "core/MWOSModule.h"

#if defined(ESP8266) || defined(ESP32)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif

#define FS_RESULT_MAX_SIZE 1024

/**
* Модуль работы с файловой системой (Версия MWOS3.5)
*
* Команды файловой системы в терминале:
*   25:LittleFS:cmd=md;папка - создать папку
*   25:LittleFS:cmd=rd;папка - удалить папку
*   25:LittleFS:cmd=del;файл - удалить файл
*   25:LittleFS:cmd=rn;файл;новый - переименовать файл
*   25:LittleFS:cmd=ls;папка - листинг файлов и подпапок в папке
*   25:LittleFS:cmd=reboot - перезагрузить контроллер
*   25:LittleFS:cmd=cfg;save - сохранить параметры в файл "/defaults.cfg"
*   25:LittleFS:cmd=cfg;load - загрузить параметры из файла "/defaults.cfg"
*   25:LittleFS:cmd=format;fs - форматировать файловую систему
*
* Параметры регистрируются автоматически через макросы MWOS_PARAM.
* Логика обработки данных объединена в onEvent:
*   EVENT_INIT - инициализация файловой системы.
*   EVENT_EMPTY_STORAGES - загрузка параметров из файлов конфигурации.
*   EVENT_CHANGE - реакция на изменение параметров.
*   EVENT_GET_VALUE - возврат значений через switch.
*/
class MWOSFS : public MWOSModule {
public:
    // указатель на файловую систему
    MWFS * fs = NULL;
    // результат выполнения команды
    String resultStr;
    // флаг отправки директории
    uint8_t sendingDir = 0;
    // флаг инициализации
    bool inited = false;
    // позиция в файле
    int16_t posFile = 0;
    // максимальный размер результата
    int16_t sizeMax = FS_RESULT_MAX_SIZE;

    // --- Объявление параметров (автоматическая регистрация) ---

    // для получения команд и ответов на них
    MWOS_PARAM(0, cmd, PARAM_STRING, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 16400);
    // общий размер файловой системы
    MWOS_PARAM(1, size, PARAM_INT64, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // используемое место файловой системы
    MWOS_PARAM(2, used, PARAM_INT64, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);
    // свободное место файловой системы
    MWOS_PARAM(3, free, PARAM_INT64, PARAM_TYPE_READONLY, MWOS_STORAGE_NO, 1);

    MWOSFS() : MWOSModule((char *) F("fs")) {
        moduleType = MODULE_FS;
    }

    MWOSFS(MWFS * _fs) : MWOSFS() {
        setFS(_fs);
    }

    /**
    * Установить файловую систему
    * @param _fs Указатель на объект файловой системы
    */
    void setFS(MWFS * _fs) {
        fs = _fs;
    }

    /**
    * Инициализировать файловую систему
    */
    void initFS() {
        if (inited || fs == NULL) return;
        name = fs->name;
        fs->init();
        uint32_t s = fs->size();
        if (s == 0) {
            bool res = fs->format();
            MW_LOG_MODULE(this); MW_LOG(F("FS format: ")); MW_LOG_LN(res);
            s = fs->size();
        }
        inited = true;
        MW_LOG_MODULE(this); MW_LOG(F("FS size: ")); MW_LOG_LN(s);
    }

    /**
    * Универсальный обработчик событий в MWOS3.5
    */
    virtual void onEvent(MWOSModeEvent modeEvent, MWValue &data) override {
        switch (modeEvent) {
            // 1. Загрузка параметров из файлов конфигурации
            case EVENT_EMPTY_STORAGES: {
                initFS();
                configFileParams(false, getFilenameCfg(0));
                configFileParams(false, getFilenameCfg(1));
                uint32_t cid = mwos.getCID();
                String fileFrom2 = String(F("/0.cfg"));
                String fileFrom3 = String(F("/u0.cfg"));
                if (cid > 0) {
                    String fileCfg = getFilenameCfg(2);
                    if (fs->exists(fileFrom2) && !fs->exists(fileCfg)) fs->rename(fileFrom2, fileCfg);
                    configFileParams(false, fileCfg);
                    fileCfg = getFilenameCfg(3);
                    if (fs->exists(fileFrom3) && !fs->exists(fileCfg)) fs->rename(fileFrom3, fileCfg);
                    configFileParams(false, fileCfg);
                } else {
                    configFileParams(false, fileFrom2);
                    configFileParams(false, fileFrom3);
                }
            } break;

            // 2. Инициализация
            case EVENT_INIT: {
                initFS();
            } break;

            // 3. Реакция на изменение конкретного параметра
            case EVENT_CHANGE: {
                // Обработка изменений параметров
            } break;

            // 4. Запрос значения
            case EVENT_GET_VALUE: {
                switch (data.param_id) {
                    case id_cmd:
                        if (resultStr.length() > (unsigned int) data.param_index) {
                            data.setValueInt(resultStr[data.param_index]);
                        } else {
                            if (sendingDir && resultStr.length() + 1 < data.param_index) nextFilename();
                            else data.setValueInt(0);
                        }
                        return;
                    case id_size:
                        if (fs != NULL) data.setValueInt(fs->size());
                        else data.setValueInt(0);
                        return;
                    case id_used:
                        if (fs != NULL) data.setValueInt(fs->used());
                        else data.setValueInt(0);
                        return;
                    case id_free:
                        if (fs != NULL) data.setValueInt(fs->available());
                        else data.setValueInt(0);
                        return;
                }
            } break;
            case EVENT_SET_VALUE: {
                if (data.param_id == id_cmd) {
                    onReceiveCommand(data);
                }
            } return;

            default:
                break;
        }

        // Вызов базового класса ОБЯЗАТЕЛЬНО в конце, после всей логики
        MWOSModule::onEvent(modeEvent, data);
    }

    /**
    * Обработка полученных команд от сервера
    */
    void onReceiveCommand(const MWValue &data) {
        String command = data.toString();
        MW_LOG_MODULE(this); MW_LOG(F("onReceiveCmd: ")); MW_LOG_LN(command);
        String args = "";
        int p = command.indexOf(';');
        if (p > 0) {
            args = command.substring(p + 1);
            command = command.substring(0, p);
        }
        if (fsCommand(command, args)) {
            if (resultStr == "") {
                if (args.length() > 100) args = args.substring(0, 100);
                resultStr = "ok: " + args;
            }
            resultStr = command + ';' + resultStr;
            MWOSModuleBase::SetParamChanged(&p_size, 0, true);
            MWOSModuleBase::SetParamChanged(&p_used, 0, true);
            MWOSModuleBase::SetParamChanged(&p_free, 0, true);
        } else {
            if (args.length() > 100) args = args.substring(0, 100);
            resultStr += String(F("error: ")) + args;
            resultStr = command + ';' + resultStr;
        }
        MWOSModuleBase::SetParamChanged(&p_cmd);
        MW_LOG_MODULE(this); MW_LOG(F("resultStr: ")); MW_LOG_LN(resultStr);
    }

    /**
    * Получить имя файла конфигурации
    * @param numCfg Номер конфигурации (0=defaults, 1=type, 2=CID, 3=user, 4=whatsnew)
    * @return Имя файла конфигурации
    */
    String getFilenameCfg(int numCfg) {
        switch (numCfg) {
            case 0: return String(F("/defaults.cfg"));
            case 1: return String(F("/t")) + String(MWOS_CONTROLLER_TYPE) + String(F(".cfg"));
            case 2: return String('/' + String(mwos.getCID()) + String(F(".cfg")));
            case 3: return String(F("/u")) + String(mwos.getCID()) + String(F(".cfg"));
            case 4: return String(F("/whatsnew.txt"));
        }
        return "";
    }

    /**
    * Выполнить команду файловой системы
    * @param command Команда
    * @param args Аргументы команды
    * @return true если команда выполнена успешно
    */
    bool fsCommand(const String &command, const String &args) {
        resultStr = "";
        if (command == "cd") return fs->chdir(args);
        if (command == "rd") return fs->rmdir(args);
        if (command == "md") return fs->mkdir(args);
        if (command == "del") return fs->remove(args);
        if (command == "format" && args == "fs") return fs->format();
        if (command == "rn") {
            int p = args.indexOf(';');
            return p > 0 && fs->rename(args.substring(0, p), args.substring(p + 1));
        }
        if (command == "reboot") {
#if defined(ESP8266) || defined(ESP32)
            ESP.restart();
#endif
        }
        if (command == "bootloader") {
#if defined(ESP8266) || defined(ESP32)
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
            esp_restart();
#endif
        }
        if (command == "cfg") {
            String fileCfg = '/' + String(mwos.getCID()) + String(F(".cfg"));
            uint32_t cfgCount = configFileParams(args == "save", fileCfg);
            if (cfgCount > 0) {
                resultStr = "cfg " + args + ": " + cfgCount;
                return true;
            }
        }
        if (command == "ls") {
            bool lsRes = fs->chdir(args);
            if (!lsRes) return false;
            sendingDir = 1;
            return true;
        }
        if (command == "dir") {
            int p = args.indexOf('*');
            String filter;
            String dir;
            if (p > 0) {
                filter = args.substring(p + 1);
                dir = args.substring(0, p);
            } else {
                filter = "";
                dir = args;
            }
            bool lsRes = fs->chdir(dir);
            if (!lsRes) return false;
            resultStr = getDirFiles(filter);
            if (resultStr == "") resultStr = " ";
            return true;
        }
        if (command == "type") {
            String fileName = findPosAndSize(args);
            if (!fs->exists(fileName)) {
                resultStr = String(F("error: Not found ")) + fileName;
                return false;
            }
            File file = fs->open(fileName, MWFILE_READ);
            if (!file) {
                resultStr = String(F("error: Dont open ")) + fileName;
                return false;
            }
            file.seek(posFile);
            resultStr = "";
            MW_LOG_MODULE(this); MW_LOG(F("file: ")); MW_LOG(file.available()); MW_LOG('/'); MW_LOG_LN(sizeMax);
            while (file.available() > 0 && resultStr.length() < sizeMax) {
                char c = file.read();
                MW_LOG('~'); MW_LOG(c);
                resultStr += c;
            }
            MW_LOG_LN();
            file.close();
            return true;
        }
        if (command == "echo") {
            int p = args.indexOf(';');
            if (p < 0) return false;
            String fileName = args.substring(0, p);
            fileName = findPosAndSize(fileName);
            File file = fs->open(fileName.c_str(), MWFILE_APPEND);
            file.print(args.substring(p + 1));
            file.flush();
            resultStr = String(F("Echo file: ")) + String(file.size());
            file.close();
            return true;
        }
        resultStr = String(F("unknown command "));
        return false;
    }

    /**
    * Найти позицию и размер в имени файла
    * @param fileName Имя файла с позицией и размером
    * @return Имя файла без позиции и размера
    */
    String findPosAndSize(const String &fileName) {
        posFile = 0;
        sizeMax = FS_RESULT_MAX_SIZE;
        String res = fileName;
        int p = fileName.indexOf(',');
        if (p > 0) {
            String posAndSize = fileName.substring(p + 1);
            res = fileName.substring(0, p);
            p = posAndSize.indexOf(',');
            if (p > 0) {
                posFile = posAndSize.substring(0, p).toInt();
                sizeMax = posAndSize.substring(p + 1).toInt();
            } else
                posFile = posAndSize.toInt();
        }
        return res;
    }

    /**
    * Сохранить/прочитать значения параметров в файле конфигурации
    * @param saveToFile Сохранить или прочитать
    * @param configFileName Имя файла конфигурации
    * @return Количество сохраненных/считанных параметров
    */
    uint32_t configFileParams(bool saveToFile, String configFileName) {
        if (!fs->exists(configFileName)) return 0;
        uint32_t res = 0;
        File fileWrite;
        File fileRead;
        if (saveToFile) {
            String filenameOld = configFileName + String(F(".old"));
            fs->remove(filenameOld);
            fs->rename(configFileName, filenameOld);
            fileRead = fs->open(filenameOld, MWFILE_READ);
            fileWrite = fs->open(configFileName, MWFILE_WRITE);
            MW_LOG_MODULE(this); MW_LOG(F("cfgFile write: ")); MW_LOG_LN(configFileName);
        } else {
            fileRead = fs->open(configFileName, MWFILE_READ);
            MW_LOG_MODULE(this); MW_LOG(F("cfgFile read: ")); MW_LOG_LN(configFileName);
        }
        while (fileRead.available() > 3) {
            wdt_reset();
            String strLine = fileRead.readStringUntil('\n');
            strLine.replace('\n', ' ');
            strLine.replace('\r', ' ');
            strLine.trim();
            if (strLine.length() > 3 && strLine[0] != ';') {
                int posR = strLine.indexOf('=');
                if (posR > 0) {
                    String paramAddress = strLine.substring(0, posR);
                    paramAddress.trim();
                    int posK = paramAddress.indexOf(':');
                    String moduleName = paramAddress.substring(0, posK);
                    moduleName.trim();
                    MWOSModule * moduleObj = (MWOSModule *) mwos.FindChildByName(moduleName.c_str());
                    if (moduleObj != NULL) {
                        int posK1 = paramAddress.indexOf(':', posK + 1);
                        String paramName;
                        String arrayIndex = "";
                        if (posK1 < 0) paramName = paramAddress.substring(posK + 1);
                        else {
                            paramName = paramAddress.substring(posK + 1, posK1);
                            arrayIndex = paramAddress.substring(posK1 + 1);
                        }
                        paramName.trim();
                        arrayIndex.trim();
                        MWOSParam * paramObj = (MWOSParam *) moduleObj->FindChildByName(paramName.c_str());
                        if (paramObj != NULL) {
                            if (saveToFile) {
                                strLine = paramAddress + " = " + moduleObj->getValueStrFromParam(paramObj, arrayIndex);
                            } else {
                                String v = strLine.substring(posR + 1);
                                v.trim();
                                if (!moduleObj->setValueStrToParam(v, paramObj, arrayIndex, true)) {
                                    MW_LOG_MODULE(this); MW_LOG(F("Save error: ")); MW_LOG_LN(strLine);
                                } else {
                                    res++;
                                }
                            }
                        } else {
                            MW_LOG_MODULE(this); MW_LOG(F("Param not found: ")); MW_LOG_LN(paramName);
                        }
                    } else {
                        MW_LOG_MODULE(this); MW_LOG(F("Module not found: ")); MW_LOG_LN(moduleName);
                    }
                }
            }
            if (saveToFile) {
                fileWrite.println(strLine);
            }
        }
        fileRead.close();
        if (saveToFile) {
            fileWrite.flush();
            fileWrite.close();
        }
        return res;
    }

    /**
    * Получить список файлов в директории
    * @param filter Фильтр по имени файла
    * @return Строка со списком файлов
    */
    String getDirFiles(String filter) {
        String res = "";
        File * file = fs->openNext();
        while (file != NULL && res.length() < FS_RESULT_MAX_SIZE) {
            String fileName = file->name();
            if (filter == "" || fileName.indexOf(filter) >= 0) {
                if (res != "") res += ";";
                if (file->isDirectory()) res += "-1";
                else res += String((int) file->size());
                res += "=" + fs->getFileName(file);
            }
            file = fs->openNext();
        }
        return res;
    }

    /**
    * Приготовить к отправке следующее имя файла
    * @return true если есть следующий файл
    */
    bool nextFilename() {
        File * file = fs->openNext();
        if (file != NULL) {
            resultStr = fs->getFileName(file) + ';';
            if (file->isDirectory()) resultStr += "-1";
            else resultStr += String((int) file->size());
            sendingDir = 1;
        } else {
            resultStr = "";
            sendingDir = 0;
        }
        MWOSModuleBase::SetParamChanged(&p_cmd);
        MW_LOG_MODULE(this); MW_LOG(F("nextFilename: ")); MW_LOG_LN(resultStr);
        return sendingDir == 1;
    }

    /**
    * Сохранить параметры в файл конфигурации
    * @return Количество сохраненных параметров
    */
    uint32_t saveConfigFileParams() {
        if (mwos.getCID() == 0) return 0;
        String fileCfg = '/' + String(mwos.getCID()) + String(F(".cfg"));
        return configFileParams(true, fileCfg);
    }

protected:
    #pragma pack(push, 1)
    // дополнительные защищённые переменные при необходимости
    #pragma pack(pop)
};

#endif