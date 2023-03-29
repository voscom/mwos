#ifndef async_ethernetclient_h
#define async_ethernetclient_h

#ifdef MWOS_NET_W5500

#include "Arduino.h"
#include "IPAddress.h"
#include "utility/w5500.h"
#include "utility/socket.h"
#include <Ethernet2.h>
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dns.h"

class AsyncEthernetClient : public EthernetClient {

public:

	int connectAsync(const char* host, uint16_t port) {
	  // Look up the host first
	  int ret = 0;
	  DNSClient dns;
	  IPAddress remote_addr;

	  dns.begin(Ethernet.dnsServerIP());
	  ret = dns.getHostByName(host, remote_addr);
	  if (ret == 1) {
	    return connectAsync(remote_addr, port);
	  } else {
	    return ret;
	  }
	}


	int connectAsync(IPAddress ip, uint16_t port) {
	  if (_sock != MAX_SOCK_NUM)
	    return 0;

	  for (int i = 0; i < MAX_SOCK_NUM; i++) {
	    uint8_t s = w5500.readSnSR(i);
	    if (s == SnSR::CLOSED || s == SnSR::FIN_WAIT || s == SnSR::CLOSE_WAIT) {
	      _sock = i;
	      break;
	    }
	  }

	  if (_sock == MAX_SOCK_NUM)
	    return 0;

	  _srcport++;
	  if (_srcport == 0) _srcport = 1024;
	  socket(_sock, SnMR::TCP, _srcport, 0);

	  if (!::connect(_sock, rawIPAddress(ip), port)) {
	    _sock = MAX_SOCK_NUM;
	    return 0;
	  }

	  while (status() != SnSR::ESTABLISHED) {
	    delay(1);
	    if (status() == SnSR::CLOSED) {
	      _sock = MAX_SOCK_NUM;
	      return 0;
	    }
	  }

	  return 1;
	}


};

#endif
#endif
