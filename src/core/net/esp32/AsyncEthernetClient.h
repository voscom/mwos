//
// Created by vladimir9@bk.ru on 26.07.2023.
// для компиляции необходима стандартная библиотека Arduino - Ethernet2
// отличается от стандарного EthernetClient из библиотеки Ethernet2 только асинхронным подключением к серверу
//

#ifndef MWOS3_ASYNCETHERNETCLIENT_H
#define MWOS3_ASYNCETHERNETCLIENT_H

#include "Arduino.h"
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"


class AsyncEthernetClient : public Client {

    public:
        AsyncEthernetClient();
        AsyncEthernetClient(uint8_t sock);

        uint8_t status();
        virtual int connect(IPAddress ip, uint16_t port);
        virtual int connect(const char *host, uint16_t port);
        virtual size_t write(uint8_t);
        virtual size_t write(const uint8_t *buf, size_t size);
        virtual int available();
        virtual int read();
        virtual int read(uint8_t *buf, size_t size);
        virtual int peek();
        virtual void flush();
        virtual void stop();
        virtual uint8_t connected();
        virtual operator bool();
        virtual bool operator==(const AsyncEthernetClient&);
        virtual bool operator!=(const AsyncEthernetClient& rhs) { return !this->operator==(rhs); };

        friend class EthernetServer;

        using Print::write;

    private:
        static uint16_t _srcport;
        uint8_t _sock;
    };


#endif //MWOS3_ASYNCETHERNETCLIENT_H
