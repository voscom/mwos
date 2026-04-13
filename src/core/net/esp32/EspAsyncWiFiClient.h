#ifndef MWASYNCSOCKETCLIENT_H
#define MWASYNCSOCKETCLIENT_H
#include <Arduino.h>
#include <Client.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <lwip/sockets.h>
  #include <lwip/netdb.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <lwip/sockets.h>
  #include <lwip/netdb.h>
  #include <fcntl.h>
#endif

/**
 * Неблокирующий сокет-клиент WiFi (ESP32 или ESP8266)
 */
class EspAsyncWiFiClient : public Client {
private:
    int8_t _sock = -1;
    bool _isConnected = false;
    uint16_t _sndBufSize; // Храним размер буфера отправки
    uint16_t _rcvBufSize; // Храним размер буфера приема

public:
    /**
     * Неблокирующий сокет-клиент WiFi (ESP32 или ESP8266)
     * @param sndBuf Можно явно задать размер буфера отправки
     * @param rcvBuf Можно явно задать размер буфера приема
     */
    EspAsyncWiFiClient(int sndBuf = 0, int rcvBuf = 0)
        : _sndBufSize(sndBuf), _rcvBufSize(rcvBuf) {}
    virtual ~EspAsyncWiFiClient() { stop(); }

    int connect(IPAddress ip, uint16_t port) override {
        stop();
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sock < 0) return 0;

        // Неблокирующий режим
        int flags = fcntl(_sock, F_GETFL, 0);
        fcntl(_sock, F_SETFL, flags | O_NONBLOCK);

        // Применяем размеры буферов из конструктора
        if (_sndBufSize > 0) {
            setsockopt(_sock, SOL_SOCKET, SO_SNDBUF, &_sndBufSize, sizeof(_sndBufSize));
        }
        if (_rcvBufSize > 0) {
            setsockopt(_sock, SOL_SOCKET, SO_RCVBUF, &_rcvBufSize, sizeof(_rcvBufSize));
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = (uint32_t)ip;

        int res = ::connect(_sock, (struct sockaddr *)&addr, sizeof(addr));
        if (res < 0 && errno != EINPROGRESS) {
            stop();
            return 0;
        }
        return 1;
    }

    int connect(const char *host, uint16_t port) override {
        IPAddress ip;
        if (WiFi.hostByName(host, ip)) return connect(ip, port);
        return 0;
    }

    uint8_t connected() override {
        if (_sock < 0) return 0;
        if (!_isConnected) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(_sock, &wfds);
            struct timeval tv = {0, 0};
            if (select(_sock + 1, NULL, &wfds, NULL, &tv) > 0) {
                int err;
                socklen_t len = sizeof(err);
                getsockopt(_sock, SOL_SOCKET, SO_ERROR, &err, &len);
                _isConnected = (err == 0);
                if (err != 0) stop();
            }
        }
        return _isConnected ? 1 : 0;
    }

    size_t write(const uint8_t *buf, size_t size) override {
        if (!connected()) return 0;
        // MSG_DONTWAIT гарантирует, что мы не зависнем, если буфер lwIP полон
        ssize_t res = send(_sock, buf, size, MSG_DONTWAIT);
        if (res < 0) return 0;
        return (size_t)res;
    }

    size_t write(uint8_t b) override { return write(&b, 1); }

    int available() override {
        if (!connected()) return 0;
        int count;
        if (ioctl(_sock, FIONREAD, &count) < 0) return 0;
        return count;
    }

    int read(uint8_t *buf, size_t size) override {
        if (!connected()) return -1;
        ssize_t res = recv(_sock, buf, size, MSG_DONTWAIT);
        if (res < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
        return res;
    }

    int read() override {
        uint8_t b;
        return (read(&b, 1) > 0) ? b : -1;
    }

    void stop() override {
        if (_sock >= 0) {
            close(_sock);
            _sock = -1;
        }
        _isConnected = false;
    }

    int peek() override {
        uint8_t b;
        if (recv(_sock, &b, 1, MSG_PEEK | MSG_DONTWAIT) > 0) return b;
        return -1;
    }

    void flush() override {}
    operator bool() override { return connected(); }
};

#endif
