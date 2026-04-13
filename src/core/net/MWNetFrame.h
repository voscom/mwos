#ifndef MWNETFRAMEREAD_H
#define MWNETFRAMEREAD_H
#include <Arduino.h>
#include "core/net/MWNetFrameSend.h"

/**
 * Создает фреймы связи. Напрямую в буфере отправки. Минимально возможный расход памяти.
 */
class MWNetFrame : public MWNetFrameSend {
public:

    // принято начало сессии
    uint8_t startSession:1;
    // отправлять данные о прошивки
    uint8_t needSendFirmware:1;
    // отправлять данные о хранилищах
    uint8_t needSendStorages:1;
    uint8_t :5;
    // таймаут отправки фреймов (1/15 сек)
    MWTimeout<uint8_t,15,true> timeoutSendFrame;

    MWNetFrame() : MWNetFrameSend() {
        startSession=false;
        needSendFirmware=false;
        needSendStorages=false;
    }

    void update() {
        if (!timeoutSendFrame.isTimeout()) return;
        sendAll();
        timeoutSendFrame.start(1); // если все не влезло - продолжим через 1/15 сек
    }

    /**
     * Отправить все, что только можно. Пока влезает в буфер.
     * @return  Влезло все (или нет)
     */
    bool sendAll() {
        if (needSendStorages) needSendStorages=SendStoragesInf() == 0;
        if (needSendStorages) return false;
        if (needSendFirmware) needSendFirmware=SendFirmwareInf() == 0;
        if (needSendFirmware) return false;
        MWOSModuleBase * module=(MWOSModuleBase *) mwos.child;
        while (module && module->unitType==UnitType::MODULE) {
            if (module->sendInf) module->sendInf=SendModuleInf(module)==0;
            if (module->sendInf) return false;
            MWOSParam * param = (MWOSParam *) module->child;
            while (param && param->unitType==UnitType::PARAM) {
                if (param->sendInf) param->sendInf=SendModuleParamInf(module,param)==0;
                if (param->sendInf) return false;
                if (startSession) {
                    if (param->changed) param->changed=SendModuleParamAllValues(module,param)==0;
                    if (param->changed) return false;
                }
                param=(MWOSParam *) param->next;
            }
            module=(MWOSModuleBase *) module->next;
        }
        return true;
    }

    // Обработать принятый фрейм. Может состоять из нескольких команд сервера.
    size_t serverCommand() {
        uint8_t * buff=_net->receivePacket.GetDataBuffer();
        int size=_net->receivePacket.GetDataSize();
        int offset=0;
        while (size>offset) {
            size_t res = serverCommand(buff,size);
            if (res==0) return offset;
            offset += res;
        }
        return offset;
    }

    uint16_t readUInt16(uint8_t * buff) {
        int16_t v;
        memcpy(&v,buff,2);
        return v;
    }

    size_t readParamValue(MWValue &data, uint8_t * buff, size_t size) {
        uint8_t cmd=buff[0];
        if (cmd>127) return 1;
        if (size<3) return 0;
        data.module_id=readUInt16(buff+1);
        if (cmd>63) return 3;
        if (size<5) return 0;
        data.param_id=readUInt16(buff+3);
        if (cmd>31) return 5;
        if (size<7) return 0;
        data.param_index=readUInt16(buff+5);
        if (cmd>15) return 7;
        if (size<9) return 0;
        uint16_t valueSize=readUInt16(buff+7);
        if (size<valueSize+9) return 0;
        data.setBuffer(buff+9,valueSize);
        return valueSize+9;
    }

    /***
     * Обработать принятую команду сервера.
     * @return  Сколько обработали
     */
    size_t serverCommand(uint8_t * buff, size_t size) {
        if (!size || !buff) return 0;
        size_t res = 0;
        mwos.mwosValue.module_id=((MWOSModuleBase *) _net)->id; // от кого
        switch (buff[0]) {
            case mwos_server_cmd_param_set_value: { // установить значение параметра с записью по месту хранения
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return 0;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return 0;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return 0;
                data.type=param->valueType;
                module->setValue(data);
            } return res;
            case mwos_server_cmd_get_format: { // отправить параметры контроллера
                needSendFirmware=true;
                needSendStorages=true;
                mwos.SetSendInfAll(3);
            } return 1;
            case mwos_server_cmd_get_firmware_inf: { // отправить параметры контроллера
                needSendFirmware=true;
                needSendStorages=true;
            } return 1;
            case mwos_server_cmd_get_format_all_modules_all_params: { // отправить форматы все параметры всех модулей
                mwos.SetSendInfAll(1);
            } return 1;
            case mwos_server_cmd_get_modules: { // отправить отдельно список модулей
                mwos.SetSendInfAll(2);
            } return 1;
            case mwos_server_cmd_start_session: { // начало сессии подключения
                startSession=true;
            } return 1;
            case mwos_server_cmd_get_all_values: { // отправить полный список всех значений
                mwos.SetChangedAll(true, false); // отправить все значения всех параметров на контроллере
            } return 1;
            case mwos_server_cmd_get_all_controls: { // отправить полный список всех управляющих значений
                mwos.SetChangedAll(true,true); // отправить все значения всех управляющих параметров на контроллере
            } return 1;
            case mwos_server_cmd_module_values: { // запросили значения всех параметров модуля
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return 0;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                module->SetParamChangedByParamId(UINT16_MAX,UINT16_MAX, true);
            } return res;
            case mwos_server_cmd_module_list_params_inf: { // запросили формат всех параметров модуля
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                module->SetParamSendInfByParamId(true);
            } return res;
            case mwos_server_cmd_module_inf: { // запросили формат модуля
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                module->sendInf=true;
            } return res;
            case mwos_server_cmd_get_all_sended: {
                // запросили все отправленные ранее параметры, на которые не пришло подтверждение
                // все отправленные ранее параметры, на которые не пришло подтверждение
                mwos.SetChangedAll(2,false);
            } return true;
            case mwos_server_cmd_param_set_log: {
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return false;
                param->saveToLog=(data.toInt()>0); // установить/снять признак сохранения параметра в журнал
            } return res;
            case mwos_server_cmd_param_set_sended: {
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return false;
                param->sended=0; // снять признак отправки (подтверждение доставки)
            } return res;
            case mwos_server_cmd_param_get_value: { // запросили значение параметра
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return false;
                module->SetParamChanged(param, data.param_index, true);
            } return res;
            case mwos_server_cmd_param_get_param: { // запросили данные всего параметра
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return false;
                module->SetParamChanged(param, UINT16_MAX, true);  // отправить все значения
            }return res;
            case mwos_server_cmd_param_inf: { // запросили формат параметра
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return false;
                param->sendInf=true;
            }return res;
            case mwos_server_cmd_param_set_param_all_values: { // установить все значения параметра
                MWValue data;
                res = readParamValue(data, buff, size);
                if (res==0) return false;
                MWOSModuleBase * module=mwos.getModule(data.module_id);
                if (!module) return false;
                MWOSParam * param=module->getParam(data.module_id);
                if (!param) return false;
                data.type=param->valueType;
                if (param->IsString()) {
                    module->setValue(data);
                } else {
                    uint16_t byteSize=param->byteSize(false);
                    for (int i = 0; i < param->arrayCount() && (i+1)*byteSize <= data.size; ++i) {
                        MWValue dataIndex(data.type,data.module_id,data.param_id,i);
                        module->setValue(dataIndex);
                    }
                }
            }return res;
        }
        return res;
    }
};

#endif
