#ifndef MWOSCMDESP_H
#define MWOSCMDESP_H

#include "MWOSCmd.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>


/**
 * Модуль соединения через ESP32 BLE.
 * Должен быть объявлен после сетевых модулей Net !
 * Принимает простые текстовые команды через Bluetooth.
 * Для ESP32C3 не работает при включенном WiFi.
 */
class MWOSCmdBLE : public MWOSCmd, BLEServerCallbacks, BLECharacteristicCallbacks {
public:

    // UUID для сервиса и характеристик
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-123456789ABC"
#define COMMAND_RX_UUID     "6E400001-B5A3-F393-E0A9-123456789ABD" // Для приема команд
#define COMMAND_TX_UUID     "6E400001-B5A3-F393-E0A9-123456789ABE" // Для отправки ответов
    BLEServer* pServer = NULL;
    BLECharacteristic* pCommandRxChar = NULL;
    BLECharacteristic* pCommandTxChar = NULL;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;
    // имя BLE (если задать пустую строку или 1 символ, то BLE не запускается)
    String _nameBLE="";
    std::string rxValue="";
    // pinPode (если =0 то без пинкода)
    uint16_t pinCode=0;

    // имя BLE (если задать пустую строку или 1 символ, то BLE не запускается)
    MWOS_PARAM(1, nameBLE, mwos_param_string, mwos_param_control, MWOS_STORAGE_NVS, 40);
    // pinPode (если =0 то без пинкода)
    MWOS_PARAM(2, pinCode, mwos_param_int16, mwos_param_control, MWOS_STORAGE_EEPROM, 1);
    // при включении включать BLE
    MWOS_PARAM(3, bleOn, mwos_param_bits1, mwos_param_option, MWOS_STORAGE_EEPROM, 1);

    MWOSCmdBLE() : MWOSCmd() {
        AddParam(&p_nameBLE);
        AddParam(&p_pinCode);
    }

    MWOSCmdBLE(Stream * stream) : MWOSCmdBLE() {
        _stream=stream;
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        switch (param->id) {
            case 1: if (arrayIndex<_nameBLE.length()) return _nameBLE[arrayIndex]; else return 0;
            case 2: return pinCode;
        }
        return MWOSModule::getValue(param, arrayIndex);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        if (modeEvent==EVENT_INIT) {
            pinCode=loadValue(pinCode,&p_pinCode,0);
            if (_nameBLE=="") _nameBLE=getProjectName()+String(MWOSNetModule::getNetModule()->getCID());
            _nameBLE=loadValueString(_nameBLE,&p_nameBLE);
            if (_nameBLE.length()>1) {
                MW_LOG_MODULE(this); MW_LOG(F("Started: ")); MW_LOG_LN(_nameBLE);
#if defined(CONFIG_IDF_TARGET_ESP32C3)
                MWOSNetModule * netDev=MWOSNetModule::getNetModule();
                if (netDev!=NULL && netDev->net_module_type==MWOSNetModule::Net_WiFi_in_ESP && netDev->IsActive()) deviceConnected = true; // не запускать BLE
                else
#endif
                    StartServerBLE();
            }
        } else
        if (modeEvent==EVENT_UPDATE) {
            if (!deviceConnected && disconnectTimeout.isStarted() && disconnectTimeout.isTimeout()) {
                BLEDevice::startAdvertising(); // Перезапускаем рекламацию
                disconnectTimeout.stop();
            }
            if (rxValue.length()>0) sendAnswer();
        }
        MWOSCmd::onEvent(modeEvent);
    }

private:
    MWTimeout disconnectTimeout;

    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
#if MWOS_LOG_CMD>2
        MW_LOG_MODULE(this); MW_LOG_LN(F("connected BLE"));
#endif
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
#if MWOS_LOG_CMD>2
        MW_LOG_MODULE(this); MW_LOG_LN(F("disconnected BLE"));
#endif
        disconnectTimeout.startMS(500);
    }

    void onWrite(BLECharacteristic* pCharacteristic) {
        rxValue = pCharacteristic->getValue();
    }

    void sendAnswer() {
        _responseStr.clear();
        if (rxValue.length() > 0) {
#if MWOS_LOG_CMD>3
            MW_LOG_MODULE(this); MW_LOG(F("Receive BLE: ")); MW_LOG_LN(rxValue.c_str());
#endif
            // Обработка команды (с ответом в _responseStr)
            for (int i = 0; i < rxValue.length(); i++) {
                readNextByte(rxValue[i]);
            }
            // Отправка ответа
            if (_responseStr.length() > 0 && deviceConnected) {
#if MWOS_LOG_CMD>3
                checkSubscriptionStatus();
                MW_LOG_MODULE(this); MW_LOG(F("Send BLE: ")); MW_LOG_LN(_responseStr);
#endif
                pCommandTxChar->setValue(_responseStr.c_str());
                pCommandTxChar->notify();
            }
        }
        rxValue.clear();
    }

#if MWOS_LOG_CMD>3
    void checkSubscriptionStatus() {
        if (!pCommandTxChar) return;

        // Получите дескриптор CCCD
        BLE2902* p2902 = (BLE2902*)pCommandTxChar->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        if (p2902) {
            bool notifyEnabled = p2902->getNotifications();
            bool indicateEnabled = p2902->getIndications();

            MW_LOG_MODULE(this);  MW_LOG(F("Subscription status - Notify: "));
            MW_LOG(notifyEnabled ? "ENABLED" : "DISABLED");
            MW_LOG(F(", Indicate: "));  MW_LOG_LN(indicateEnabled ? "ENABLED" : "DISABLED");
        } else {
            MW_LOG_MODULE(this); MW_LOG_LN(F("ERROR: BLE2902 descriptor not found!"));
        }
    }
#endif

    // название подпроекта
    String getProjectName() {
        String projName=MWOS_PROJECT;
        int p1=projName.indexOf('.');
        if (p1>0) projName=projName.substring(p1+1);
        return projName;
    }

    void StartServerBLE() {
        // Инициализация BLE
        BLEDevice::init(_nameBLE.c_str());

        // Создание сервера
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);

        // Создание сервиса
        BLEService *pService = pServer->createService(SERVICE_UUID);

        // Характеристика для приема команд
        pCommandRxChar = pService->createCharacteristic(
            COMMAND_RX_UUID,
            BLECharacteristic::PROPERTY_WRITE
        );
        pCommandRxChar->setCallbacks(this);

        // Характеристика для отправки ответов (с уведомлениями)
        pCommandTxChar = pService->createCharacteristic(
            COMMAND_TX_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        pCommandTxChar->addDescriptor(new BLE2902());

        // Настройка рекламы
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        //pAdvertising->setScanResponse(true);

        BLEAdvertisementData advertisementData;
        advertisementData.setFlags(0x06); // GENERAL_DISC_MODE | BR_EDR_NOT_SUPPORTED
        advertisementData.setServiceData(BLEUUID((uint16_t)0x574D), getProjectName().c_str());

        pAdvertising->setAdvertisementData(advertisementData);
        pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x574D));

        // Параметры для лучшей совместимости
        pAdvertising->setMinPreferred(0x06);  // Минимальный интервал
        pAdvertising->setMaxPreferred(0x12);  // Максимальный интервал

        // Запуск сервиса
        pService->start();
        // Запуск рекламы
        BLEDevice::startAdvertising();
        _status=1;
        SetParamChanged(&p_status);
#if MWOS_LOG_CMD>1
        MW_LOG_MODULE(this); MW_LOG(F("Started BLE: ")); MW_LOG_LN(_nameBLE);
#endif
    }


};


#endif //MWOSCMDESP_H
