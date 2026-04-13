#ifndef MWASYNCCLIENT_H_
#define MWASYNCCLIENT_H_
#if defined(ESP32) || defined(ESP8266)
#include "Client.h"

#ifndef ASYNC_TCP_SSL_ENABLED
#define ASYNC_TCP_SSL_ENABLED 0
#endif

class cbuf;
class AsyncClient;

class MWAsyncClient: public Client {
  private:
    AsyncClient *_client;
    cbuf *_tx_buffer;
    size_t _tx_buffer_size;
    cbuf *_rx_buffer;

    size_t _sendBuffer();
    void _onData(void *data, size_t len);
    void _onConnect(AsyncClient *c);
    void _onDisconnect();
    void _attachCallbacks();
    void _attachCallbacks_Disconnect();
    void _attachCallbacks_AfterConnected();

  public:
    MWAsyncClient(size_t txBufLen = 1460);
    MWAsyncClient(AsyncClient *client, size_t txBufLen = 1460);
    virtual ~MWAsyncClient();

    virtual operator bool(){ return connected(); }
    MWAsyncClient & operator=(const MWAsyncClient &other);

#if ASYNC_TCP_SSL_ENABLED
    virtual int connect(IPAddress ip, uint16_t port, bool secure);
    virtual int connect(const char *host, uint16_t port, bool secure);
    virtual int connect(IPAddress ip, uint16_t port){
      return connect(ip, port, false);
    }
    virtual int connect(const char *host, uint16_t port){
      return connect(host, port, false);
    }
#else
    virtual int connect(IPAddress ip, uint16_t port);
    virtual int connect(const char *host, uint16_t port);

    virtual int connect(IPAddress ip, uint16_t port, int timeout) {
    	return connect(ip,port);
    }
    virtual int connect(const char *host, uint16_t port, int timeout) {
        return connect(host,port);
    }

#endif
    virtual void setTimeout(uint32_t seconds);

    virtual uint8_t status();
    virtual uint8_t connected();
    virtual void stop();

    virtual size_t write(uint8_t data);
    virtual size_t write(const uint8_t *data, size_t len);

    int writeAvailable();
    virtual int availableForWrite() { return writeAvailable(); }

    virtual int available();
    virtual int peek();
    virtual int read();
    virtual int read(uint8_t *data, size_t len);
    virtual void flush();
};

#endif
#endif /* MWASYNCCLIENT_H_ */
