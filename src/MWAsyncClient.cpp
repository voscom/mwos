#if defined(ESP32) || defined(ESP8266)
#include "MWAsyncClient.h"

#include "Arduino.h"
#if defined(ESP8266)
#include "ESPAsyncTCP.h"
#else
#include "AsyncTCP.h"
#endif
#include "cbuf.h"


MWAsyncClient::MWAsyncClient(size_t txBufLen)
  : _client(NULL)
  , _tx_buffer(NULL)
  , _tx_buffer_size(txBufLen)
  , _rx_buffer(NULL)
{}

MWAsyncClient::MWAsyncClient(AsyncClient *client, size_t txBufLen)
  : _client(client)
  , _tx_buffer(new cbuf(txBufLen))
  , _tx_buffer_size(txBufLen)
  , _rx_buffer(NULL)
{
  _attachCallbacks();
}

MWAsyncClient::~MWAsyncClient(){
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  while(_rx_buffer != NULL){
    cbuf *b = _rx_buffer;
    _rx_buffer = _rx_buffer->next;
    delete b;
  }
}

int MWAsyncClient::writeAvailable() {
	if(_tx_buffer == NULL) return 0;
	return _tx_buffer->room();
}

#if ASYNC_TCP_SSL_ENABLED
int MWAsyncClient::connect(IPAddress ip, uint16_t port, bool secure){
#else
int MWAsyncClient::connect(IPAddress ip, uint16_t port){
#endif
    if (_client != NULL && !_client->connected() && !_client->disconnecting()) return 1;
    if(_client != NULL && connected()) return 0;
  _client = new AsyncClient();
  _client->onConnect([](void *obj, AsyncClient *c){ ((MWAsyncClient*)(obj))->_onConnect(c); }, this);
  _attachCallbacks_Disconnect();
#if ASYNC_TCP_SSL_ENABLED
  if(_client->connect(ip, port, secure)){
#else
  if(_client->connect(ip, port)){
#endif
    //while(_client != NULL && !_client->connected() && !_client->disconnecting()) delay(1);
    return 1; //connected();
  }
  return 0;
}

#if ASYNC_TCP_SSL_ENABLED
int MWAsyncClient::connect(const char *host, uint16_t port, bool secure){
#else
int MWAsyncClient::connect(const char *host, uint16_t port){
#endif
    if (_client != NULL && !_client->connected() && !_client->disconnecting()) return 1;
	if(_client != NULL && connected()) return 0;
  _client = new AsyncClient();
  _client->onConnect([](void *obj, AsyncClient *c){ ((MWAsyncClient*)(obj))->_onConnect(c); }, this);
  _attachCallbacks_Disconnect();
#if ASYNC_TCP_SSL_ENABLED
  if(_client->connect(host, port, secure)){
#else
  if(_client->connect(host, port)){
#endif
	  //Serial.println(F("Async connect to voscom.ru"));
    //while(_client != NULL && !_client->connected() && !_client->disconnecting()) delay(1);
    return 1; //connected();
  }
  return 0;
}

MWAsyncClient & MWAsyncClient::operator=(const MWAsyncClient &other){
  if(_client != NULL){
    _client->abort();
    _client->free();
    _client = NULL;
  }
  _tx_buffer_size = other._tx_buffer_size;
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  while(_rx_buffer != NULL){
    cbuf *b = _rx_buffer;
    _rx_buffer = b->next;
    delete b;
  }
  _tx_buffer = new cbuf(other._tx_buffer_size);
  _client = other._client;
  _attachCallbacks();
  return *this;
}

void MWAsyncClient::setTimeout(uint32_t seconds){
  if(_client != NULL)
    _client->setRxTimeout(seconds);
}

uint8_t MWAsyncClient::status(){
  if(_client == NULL)
    return 0;
  return _client->state();
}

uint8_t MWAsyncClient::connected(){
	return (_client != NULL) && _client->connected() && !_client->disconnecting();
	//return (_client != NULL && _client->connected());
}

void MWAsyncClient::stop(){
  if(_client != NULL)
    _client->close(true);
}

size_t MWAsyncClient::_sendBuffer(){
  size_t available = _tx_buffer->available();
  if(!connected() || !_client->canSend() || available == 0)
    return 0;
  size_t sendable = _client->space();
  if(sendable < available)
    available= sendable;
  char *out = new char[available];
  _tx_buffer->read(out, available);
  size_t sent = _client->write(out, available);
  delete[] out;
  return sent;
}

void MWAsyncClient::_onData(void *data, size_t len){
  _client->ackLater();
  cbuf *b = new cbuf(len+1);
  if(b != NULL){
    b->write((const char *)data, len);
    if(_rx_buffer == NULL)
      _rx_buffer = b;
    else {
      cbuf *p = _rx_buffer;
      while(p->next != NULL)
        p = p->next;
      p->next = b;
    }
  }
}

void MWAsyncClient::_onDisconnect(){
  if(_client != NULL){
    _client = NULL;
  }
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
}

void MWAsyncClient::_onConnect(AsyncClient *c){
  _client = c;
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  _tx_buffer = new cbuf(_tx_buffer_size);
  _attachCallbacks_AfterConnected();
}

void MWAsyncClient::_attachCallbacks(){
  _attachCallbacks_Disconnect();
  _attachCallbacks_AfterConnected();
}

void MWAsyncClient::_attachCallbacks_AfterConnected(){
  _client->onAck([](void *obj, AsyncClient* c, size_t len, uint32_t time){ ((MWAsyncClient*)(obj))->_sendBuffer(); }, this);
  _client->onData([](void *obj, AsyncClient* c, void *data, size_t len){ ((MWAsyncClient*)(obj))->_onData(data, len); }, this);
  _client->onTimeout([](void *obj, AsyncClient* c, uint32_t time){ c->close(); }, this);
}

void MWAsyncClient::_attachCallbacks_Disconnect(){
  _client->onDisconnect([](void *obj, AsyncClient* c){ ((MWAsyncClient*)(obj))->_onDisconnect(); delete c; }, this);
}

size_t MWAsyncClient::write(uint8_t data){
  return write(&data, 1);
}

size_t MWAsyncClient::write(const uint8_t *data, size_t len){
  if(_tx_buffer == NULL || !connected()){
    return 0;
  }
  size_t toWrite = 0;
  size_t toSend = len;
  while(_tx_buffer->room() < toSend){
    toWrite = _tx_buffer->room();
    _tx_buffer->write((const char*)data, toWrite);
    while(!_client->canSend() && connected())
      delay(0);
    _sendBuffer();
    toSend -= toWrite;
  }
  _tx_buffer->write((const char*)(data+(len - toSend)), toSend);
  if(_client->canSend() && connected())
    _sendBuffer();
  return len;
}

int MWAsyncClient::available(){
  if(_rx_buffer == NULL) return 0;
  size_t a = 0;
  cbuf *b = _rx_buffer;
  while(b != NULL){
    a += b->available();
    b = b->next;
  }
  return a;
}

int MWAsyncClient::peek(){
  if(_rx_buffer == NULL) return -1;
  return _rx_buffer->peek();
}

int MWAsyncClient::read(uint8_t *data, size_t len){
  if(_rx_buffer == NULL) return -1;

  size_t readSoFar = 0;
  while(_rx_buffer != NULL && (len - readSoFar) >= _rx_buffer->available()){
    cbuf *b = _rx_buffer;
    _rx_buffer = _rx_buffer->next;
    size_t toRead = b->available();
    readSoFar += b->read((char*)(data+readSoFar), toRead);
    if(connected()){
        _client->ack(b->size() - 1);
    }
    delete b;
  }
  if(_rx_buffer != NULL && readSoFar < len){
    readSoFar += _rx_buffer->read((char*)(data+readSoFar), (len - readSoFar));
  }
  return readSoFar;
}

int MWAsyncClient::read(){
  uint8_t res = 0;
  if(read(&res, 1) != 1)
    return -1;
  return res;
}

void MWAsyncClient::flush(){
  if(_tx_buffer == NULL || !connected())
    return;
  if(_tx_buffer->available()){
    while(!_client->canSend() && connected())
      delay(0);
    _sendBuffer();
  }
}
#endif