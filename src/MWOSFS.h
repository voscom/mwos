#ifndef MWOS3_MWOSFS_H
#define MWOS3_MWOSFS_H

enum MWOSFS_ReciveStep: uint8_t {
    RECIVE_None=0,
    RECIVE_Cmd=1,
    RECIVE_FileName=2,
    RECIVE_FileNum=3,
    RECIVE_Block=4,
};

enum MWOSFS_ReciveCmd: uint8_t {
    CMD_OpenFile=0,
    CMD_ReadFile=1,
    CMD_File=2,
};

/**
 * Модуль работы с файловой системой
 */
class MWOSFS : public MWOSModule {
public:

    MWOS_PARAM(0, fs, mwos_param_string, mwos_param_option+mwos_param_readonly, mwos_param_storage_no, 1);
    MWOS_PARAM(1, size, mwos_param_int64, mwos_param_option+mwos_param_readonly, mwos_param_storage_no, 1);
    MWOS_PARAM(2, used, mwos_param_int64, mwos_param_option+mwos_param_readonly, mwos_param_storage_no, 1);
    MWOS_PARAM(3, free, mwos_param_int64, mwos_param_option+mwos_param_readonly, mwos_param_storage_no, 1);

    MWOSFS() : MWOSModule((char *) F("fs")) {
    }

    MWOSFS(MWFS * _fs) : MWOSModule((char *) F("fs")) {
        setFS(_fs);
    }

    void setFS(MWFS * _fs) {
        fs=_fs;
    }

    virtual void onInit() {
        if (fs==NULL) return;
        fs->init();
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (fs!=NULL)
        switch (param->id) { // для скорости отправим текущие значения из локальнх переменных
            case 1: return fs->size();
            case 2: return fs->used();
            case 3: return fs->available();
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    virtual bool onReciveCmd(MWOSProtocolCommand cmd, MWOSNetReciverFields * reciverDat) {
        if (!MWOSModule::onReciveCmd(cmd,reciverDat)) { // это не стандартное значение
            if (reciveStep>0) {  // идет прием блока побайтно
                if (cmd<0) {
                    reciveStep= RECIVE_None; // любая новая команда приводит к заершению блока данных
                } else {
                    switch (reciveStep) {
                        case RECIVE_Cmd:
                            reciveCmd=cmd;
                            fileName="";
                            if ((MWOSFS_ReciveCmd) cmd==CMD_OpenFile) { // команда открытия файла: потом идет имя файла и байт атрибута (0-READ,1-WRITE)
                                reciveStep=RECIVE_FileName;
                            }
                            break;
                        case RECIVE_FileName:
                            if (cmd<32) {
                                uint8_t fileAttr=FILE_READ;
                                if (cmd==1) fileAttr=FILE_WRITE;
                                if (openFile.isOpen()) openFile.close();
                                openFile=fs->open(fileName,fileAttr);
                                fileName="";
                            }
                            fileName+=(char) cmd;
                            break;
                    }


                }
            }
            if (cmd == mwos_server_cmd_param_start_block) reciveStep = RECIVE_Cmd; // начнем прием блока побайтно

        }
        return false;
    }

private:
    File openFile;
    String fileName;
    MWFS * fs= NULL;
    uint8_t reciveStep= RECIVE_None;
    uint8_t reciveCmd;
};


#endif //MWOS3_MWOSFS_H
