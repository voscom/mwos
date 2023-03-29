#ifndef MWOS3_MWOSNETRESIVER_H
#define MWOS3_MWOSNETRESIVER_H

/***
 * Модуль приема и обработки сетевых данных
 *
 *     Чтение поточно (без буффера) от сервера контроллеру:
    4b: {03"MW"03} - признак начала пакета.
    1b: {команда от сервера} MWOSProtocolCommand (MWOSConsts.h)
    2b: {id модуля} - начинаем считать контрольную сумму (включая id модуля) (если код команды > 63, то блок закончился)
    2b: {id параметра} - (если код команды > 31, то блок закончился)

    - если передана команда mwos_server_cmd_route, то все дальнейшие данные передаются указанному модулю типа gate
    - (при необходимости адресации реального параметра модуля gate - должна быть передана любая команда, кроме mwos_server_cmd_route)
    - модуль типа gate интерпретирует {id параметра} как внутренний id подчиненного контроллера (даже если модуль gate имеет параметры с таким кодом)
    - и далее все данные пересылает на этот подчиненный контроллер (предварительно послав {03"MW"03} - признак начала пакета)
    - заменяя в конце контрольную сумму блока на новую. Таким образом, подчиненный контроллер получает стандартную посылку
    - он, в свою очередь, тоже может иметь модули типа gate с подчиненной сетью

    2b: {индекс массива} - если параметр не массив, то всегда 0 (если код команды > 15, то блок закончился)
    2b: {длина блока, начиная со следующего байта} - тут делаем модулю reciveStart({id параметра},{длина блока})
    далее, каждый байт блока передаем модулю recive({байт})
    когда закончилась длина блока - считаем контрольную сумму
    2b: {контрольная сумма блока}
    вызываем модулю reciveStop() если контрольная сумма совпала
    если контрольная сумма не совпала, или связь прервалась - вызываем модулю reciveError({код ошибки})

 *   При чтении длина блока передается сразу после id параметра
 *
 *
 *
 */

#include <core/MWOSConsts.h>
#include "MWOSNetTransmitter.h"
#include "MWOSNetReciverFields.h"


#ifndef MWOS_RECIVE_BLOCK_TIMEOUT_DSEC
#define MWOS_RECIVE_BLOCK_TIMEOUT_DSEC 10 // Таймаут во время приема блока (сек/10)
#endif

#ifndef MWOS_CONTROLLER_TYPE
#define MWOS_CONTROLLER_TYPE 0 // Тип контроллера `controller`.`type`
#endif

class MWOSNetReciver : public MWOSNetTransmitter {
public:

#pragma pack(push,1)
    MWOSNetReciverFields recive_args;
    MWOSProtocolCommand cmd; // полученная от сервера команда mwos_server_cmd_*
    uint8_t _cmdMode=0;
    uint16_t reciveBlockSize;
    MW_CRC16 recive_crc16;
    MWTimeout recive_block_timeout;
    MWTimeout lastReciveTimeout;
    // для ускорения алгоритма приема
    MWOSParam * reciveParam;
    MWOSModule * reciveModule;
#pragma pack(pop)

    /**
     * Обрабатывает очередной байт из стрима, на предмет команды для контроллера
     * Все передачи данных для любых контроллеров всегда начинаются с подписи "MW\0x3"
     * затем идет двухбайтовая длина блока, а потом уже все остальное
     * Если обнаружена подпись и считана длина, то _cmdMode == 5
     */
    virtual void readNexByte(uint8_t reciveByte) {
        switch (_cmdMode) {
            case 0: // начало пакета
                if (reciveByte==03) {
                    _cmdMode++;
                    recive_block_timeout.start(MWOS_RECIVE_BLOCK_TIMEOUT_DSEC); // задать таймаут приема блока
                }
                return;
            case 1: // начало пакета
                if (reciveByte=='M') {
                    _cmdMode++;
                }
                return;
            case 2: // начало пакета
                if (reciveByte=='W') _cmdMode++;
                else reciveClear(_cmdMode);
                return;
            case 3: // начало пакета
                if (reciveByte==03) _cmdMode++;
                else reciveClear(_cmdMode);
                return;
            case 4: // команда от сервера
                recive_crc16.start();
                recive_crc16.add(reciveByte);
                cmd=(MWOSProtocolCommand) reciveByte;
                if (cmd>127) { // это команда контроллеру (без аргументов)
                    _cmdMode=14; // сразу перейти к расчету контрольной суммы
                    return;
                }
                _cmdMode++;
                return;
            case 5: // id модуля
                recive_args.module_id=reciveByte;
                recive_crc16.add(reciveByte);
                _cmdMode++;
                return;
            case 6: //  id модуля
                recive_args.module_id |=reciveByte << 8;
                //MW_LOG(F("Recive module id ")); MW_LOG_LN(reciveModule_id);
                reciveModule=(MWOSModule *) mwos.getModule(recive_args.module_id);
                if (reciveModule!=NULL) {
                    recive_crc16.add(reciveByte);
                    if (cmd>63) { // это команда модулю (без аргументов)
                        _cmdMode=14; // сразу перейти к расчету контрольной суммы
                        return;
                    }
                    _cmdMode++;
                } else reciveClear(_cmdMode);
                return;
            case 7: // младший байт id параметра
                recive_args.param_id=reciveByte;
                recive_crc16.add(reciveByte);
                _cmdMode++;
                return;
            case 8: // старший байт id параметра
                recive_args.param_id |=reciveByte << 8;
                //MW_LOG(F("Recive param id ")); MW_LOG_LN(reciveParam_id);
                reciveParam=reciveModule->getParam(recive_args.param_id);
                if (reciveParam!=NULL) {
                    recive_crc16.add(reciveByte);
                    if (cmd>31) { // это команда параметру (без аргументов)
                        _cmdMode=14; // сразу перейти к расчету контрольной суммы
                        return;
                    }
                    recive_args.valueIsLong=reciveParam->IsLong();
                    _cmdMode++;
                } else reciveClear(_cmdMode);
                return;
            case 9: // младший байт индекса массива
                recive_args.array_index=reciveByte;
                recive_crc16.add(reciveByte);
                _cmdMode++;
                return;
            case 10: // старший байт индекса массива
                recive_args.array_index |=reciveByte << 8;
                //MW_LOG(F("Recive index ")); MW_LOG_LN(recive_param_array_index);
                if (recive_args.array_index<reciveParam->arrayCount()) {
                    recive_crc16.add(reciveByte);
                    if (cmd>15) { // это команда параметру с индексом массива (без аргументов)
                        _cmdMode=14; // сразу перейти к расчету контрольной суммы
                        return;
                    }
                    _cmdMode++;
                } else reciveClear(_cmdMode);
                return;
            case 11: // размер данных (младший байт)
                reciveBlockSize=reciveByte;
                recive_crc16.add(reciveByte);
                _cmdMode++;
                return;
            case 12: // размер данных (старший байт)
                reciveBlockSize |=reciveByte << 8;
                if ((reciveBlockSize >= MWOS_RECIVE_BLOCK_SIZE) || (reciveBlockSize < 1)) {
                    reciveClear(_cmdMode);
                    return;
                }
                recive_crc16.add(reciveByte);
                _cmdMode++;
                // начнем прием данных
                //recive_param_size=module->getParam(reciveParam_id)->byteSize(false); // размер в байтах
                if (recive_args.valueIsLong) {
                    //recive_param_storage=module->getParam(reciveParam_id)->getStorage();
                    //module->onReciveStart(reciveCmd,reciveParam_id,reciveBlockSize);
#if MWOS_RECIVE_BLOCK_SIZE>=0xffff
                    recive_args.offset=reciveBlockSize; // временно зададим в смещение длину блока для старта приема
                    reciveModule->onReciveCmd(mwos_server_cmd_param_start_block,&recive_args);
#endif
                    //recive_param_bitOffset=module->getParamBitOffset(reciveParam_id,0);
                } else {
                    recive_args.reciveValue=0;
                }
                recive_args.offset=0; // начнем принимать блок сначала
                return;
            case 13: // аргументы
                recive_crc16.add(reciveByte);
                if (recive_args.valueIsLong) {
                    // если лезет в параметр - запишем
                    //if (recive_offset<recive_param_size) mwos.saveValue(reciveByte,recive_param_storage,recive_param_bitOffset+(recive_offset<<3),8, false);
                    //mwos.getModule(reciveModule_id)->onRecive(reciveByte,recive_offset);
#if MWOS_RECIVE_BLOCK_SIZE<0xffff
                    if (recive_args.offset<MWOS_RECIVE_BLOCK_SIZE) recive_args.buffer[recive_args.offset]=reciveByte;
                    else reciveClear(_cmdMode);
#else
                    recive_args.buffer[0]=reciveByte;
                    reciveModule->onReciveCmd((MWOSProtocolCommand) reciveByte, &recive_args);
#endif
                } else {
                    if (recive_args.offset<8) {
                        uint8_t * recBlock=(uint8_t *) &recive_args.reciveValue;
                        recBlock[recive_args.offset]=reciveByte;
                    }
                    //MW_LOG(F("Recive arg ")); MW_LOG_LN(reciveByte);
                }
                reciveBlockSize--;
                recive_args.offset++;
                if (reciveBlockSize==0) _cmdMode++;
                return;
            case 14: // контрольная сумма (младший байт)
                reciveBlockSize=reciveByte;
                _cmdMode++;
                return;
            case 15: // контрольная сумма  (старший байт)
                reciveBlockSize |=reciveByte << 8;
                int8_t errorCode=0;
                if (reciveBlockSize!=recive_crc16.crc) {
                    errorCode=_cmdMode;
                    MW_LOG(F("Recive CRC Error ")); MW_LOG(reciveBlockSize); MW_LOG('='); MW_LOG_LN(recive_crc16.crc);
                    MW_LOG(F("Error command ")); MW_LOG(cmd); MW_LOG('>'); MW_LOG(recive_args.module_id);
                    MW_LOG(':'); MW_LOG(recive_args.param_id);  MW_LOG(':'); MW_LOG(recive_args.array_index);
                    MW_LOG('='); MW_LOG_LN((int32_t) recive_args.reciveValue);
                } else {
                    MW_LOG(F("Recive command ")); MW_LOG(cmd); MW_LOG('>'); MW_LOG(recive_args.module_id);
                        MW_LOG(':'); MW_LOG(recive_args.param_id);  MW_LOG(':'); MW_LOG(recive_args.array_index);
                        MW_LOG('='); MW_LOG_LN((int32_t) recive_args.reciveValue);
                    if (serverCommand()) { // это команда сервера
                    }
                    lastReciveTimeout.start(MWOS_RECIVE_TIMEOUT_DSEC);
                }
                if (recive_args.valueIsLong) {
                    MW_LOG(F("Recive data ")); MW_LOG(recive_args.module_id);
                        MW_LOG(':'); MW_LOG(recive_args.param_id); MW_LOG('='); MW_LOG(recive_args.array_index);
                        MW_LOG(F(", error:")); MW_LOG_LN(errorCode);
                    //mwos.getModule(reciveModule_id)->onReciveStop(reciveParam_id, recive_offset, errorCode);
#if MWOS_RECIVE_BLOCK_SIZE<0xffff
                    if (errorCode==0) reciveModule->onReciveCmd(cmd,&recive_args);
#else
                    if (errorCode==0) reciveModule->onReciveCmd(mwos_server_cmd_param_stop_block,&recive_args);
                    else reciveModule->onReciveCmd(mwos_server_cmd_param_error_block,&recive_args);
#endif
                }
                reciveClear(errorCode);
                return;
        }
    }

    /***
     * Обработать принятую команду сервера
     * @return
     */
    bool serverCommand() {
        switch (cmd) {
            case mwos_server_cmd_param_save: // записать текущее значение параметра по месту хранения
                reciveModule->saveValue(
                        reciveModule->getValueByParamID(recive_args.param_id,recive_args.array_index),
                        recive_args.param_id,recive_args.array_index); // это значение в пределах 8 байт
                return true;
            case mwos_server_cmd_param_reload: // перезагрузить текущее значение из места хранения
                reciveModule->setValueByParamID(
                        reciveModule->loadValue(0,recive_args.param_id,recive_args.array_index),
                        recive_args.param_id,recive_args.array_index); // это значение в пределах 8 байт
                return true;
            case mwos_server_cmd_get_format: // отправить параметры контроллера
                sendFormatController();
                return true;
            default:
                if (cmd<mwos_server_cmd_get_format) { // команды модулю (и их параметрам)
                    return reciveModule->onReciveCmd(cmd,&recive_args);
                }
        }
        return false;
    }

    void reciveClear(uint8_t code) {
        if (code>0) {
            MW_LOG(F("reciveClear: ")); MW_LOG(_cmdMode); MW_LOG(';'); MW_LOG_LN(code);
        }
        _cmdMode=0;
        reciveBlockSize=0;
        recive_block_timeout.stop();
    }


};


#endif //MWOS3_MWOSNETRESIVER_H
