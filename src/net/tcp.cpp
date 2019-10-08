#include <net/tcp.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;


TransmissionControlProtocolProvider::TransmissionControlProtocolProvider(InternetProtocolV4Provider* backend)
: InternetProtocolV4Handler(backend, 0x06)
{
  this->numSockets = 0;
  for(uint32_t i = 0; i < 65535; i++) {
    sockets[i] = 0;
  }
  this->freePort = 1024;
}

TransmissionControlProtocolSocket* TransmissionControlProtocolProvider::connect(uint32_t ip, uint16_t port, uint16_t localPort) {
  this->connect(ip, port, 0);
}

TransmissionControlProtocolSocket* TransmissionControlProtocolProvider::connect(uint32_t ip, uint16_t port, uint16_t localPort) {
  TransmissionControlProtocolSocket* socket = new TransmissionControlProtocolSocket(this);
  
  socket->state = SYN_SENT;
  socket->sequenceNumber = 302342478; // This number should be randomly generated.
  
  socket->remotePort = port;
  socket->remoteIp = ip;
  
  socket->localPort = localPort;
  if(localPort == 0) {
    socket->localPort = freePort++;
    if(freePort == 65530) freePort = 1024;
  }
  
  socket->forwardBroadcasts = false;
  socket->forwardAll = false;
  socket->localIp = backend->getIpAddress();
  
  socket->remotePort = ((socket->remotePort & 0xFF00) >> 8) | ((socket->remotePort & 0x00FF) << 8);
  socket->localPort = ((socket->localPort & 0xFF00) >> 8) | ((socket->localPort & 0x00FF) << 8);
  
  this->sockets[numSockets] = socket;
  this->numSockets++;
  
  this->sendTCP(socket, 0, 0, SYN);
  
  return socket;
}

TransmissionControlProtocolSocket* TransmissionControlProtocolProvider::listen(common::uint16_t port) {
  UserDatagramProtocolSocket* socket = new UserDatagramProtocolSocket(this);
  
  socket->state = LISTENING;
  socket->localIp = backend->getIpAddress();
  socket->remotePort = ((port & 0xFF00) >> 8) | ((port & 0x00FF) << 8);
  
  this->sockets[numSockets] = socket;
  this->numSockets++;
  
  return socket;
}

void TransmissionControlProtocolProvider::disconnect(TransmissionControlProtocolSocket* socket) {
  socket->state = FIN_WAIT1;
  
  this->sendTCP(socket, 0, 0, FIN + ACK);
  
  socket->sequenceNumber++;
}
