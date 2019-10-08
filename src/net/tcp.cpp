#include <net/tcp.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;

uint32_t bigEndian32(uint32_t n) {
  return ((n & 0xFF000000) >> 24)
       | ((n & 0x00FF0000) >> 8)
       | ((n & 0x0000FF00) << 8)
       | ((n & 0x000000FF) << 24);
}

TransmissionControlProtocolHandler::TransmissionControlProtocolHandler() {}
TransmissionControlProtocolHandler::~TransmissionControlProtocolHandler() {}

void TransmissionControlProtocolHandler::handleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, uint8_t* data, uint32_t length) {}

TransmissionControlProtocolSocket::TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend) {
  this->state = CLOSED;
  this->backend = backend;
  this->handler = 0;
}
TransmissionControlProtocolSocket::~TransmissionControlProtocolSocket() {}

void TransmissionControlProtocolSocket::handleUserDatagramProtocolMessage(uint8_t* data, uint32_t length) {}

void TransmissionControlProtocolSocket::send(uint8_t* data, uint16_t length) {
  this->backend->sendTCP(this, data, length);
}

void TransmissionControlProtocolSocket::setHandler(TransmissionControlProtocolHandler* handler) {
  this->handler = handler;
}

void TransmissionControlProtocolSocket::disconnect() {
  backend->disconnect(this);
}

TransmissionControlProtocolProvider::TransmissionControlProtocolProvider(InternetProtocolV4Provider* backend)
: InternetProtocolV4Handler(backend, 0x06)
{
  this->numSockets = 0;
  for(uint32_t i = 0; i < 65535; i++) {
    sockets[i] = 0;
  }
  this->freePort = 1024;
}

TransmissionControlProtocolProvider::~TransmissionControlProtocolProvider() {}

bool TransmissionControlProtocolProvider::onInternetProtocolReceived(uint32_t srcIp_BE, uint32_t destIp_BE, uint8_t* payload, uint32_t size) {}

void TransmissionControlProtocolProvider::sendTCP(TransmissionControlProtocolSocket* socket, uint8_t* data, uint16_t length, uint16_t flags) {
  uint16_t packetLength = length + sizeof(TransmissionControlProtocolHeader); // Needed for pseudoHeader.
  
  uint16_t totalLength = length + sizeof(TransmissionControlProtocolHeader) + sizeof(TransmissionControlProtocolPseudoHeader);
  uint8_t* buffer = (uint8_t*) MemoryManager::activeMemoryManager->malloc(totalLength);
  
  TransmissionControlProtocolPseudoHeader* pseudoHeader = (TransmissionControlProtocolPseudoHeader*) buffer;
  TransmissionControlProtocolHeader* header = (TransmissionControlProtocolHeader*) (buffer + sizeof(TransmissionControlProtocolPseudoHeader));
  uint8_t* payload = buffer + sizeof(TransmissionControlProtocolHeader) + sizeof(TransmissionControlProtocolPseudoHeader);
  
  header->headerSize32 = sizeof(TransmissionControlProtocolHeader)/4;
  header->srcPort = socket->localPort;
  header->destPort = socket->remotePort;
  
  header->sequenceNumber = bigEndian32(socket->sequenceNumber);
  header->acknowledgementNumber = bigEndian32(socket->acknowledgementNumber);
  
  header->reserved = 0;
  header->flags = flags;
  header->windowSize = 0xFFFF;
  header->urgent = 0;
  
  header->options = ((flags & SYN) != 0) ? 0xB4050402 : 0;
  
  socket->sequenceNumber += length;
  
  for(int i = 0; i < length; i++) {
    payload[i] = data[i];
  }
  
  pseudoHeader->srcIP = socket->localIp;
  pseudoHeader->destIP = socket->remoteIp;
  pseudoHeader->protocol = 0x0600; // BE 6.
  pseudoHeader->totalLength = ((packetLength & 0x00FF) << 8) | ((packetLength & 0xFF00) >> 8);
  
  header->checksum = 0;
  header->checksum = InternetProtocolV4Provider::checksum((uint16_t*) buffer, totalLength);
  
  this->send(socket->remoteIp, (uint8_t*) header, packetLength);
  
  MemoryManager::activeMemoryManager->free(buffer);
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
  
  socket->localIp = backend->getIpAddress();
  
  socket->remotePort = ((socket->remotePort & 0xFF00) >> 8) | ((socket->remotePort & 0x00FF) << 8);
  socket->localPort = ((socket->localPort & 0xFF00) >> 8) | ((socket->localPort & 0x00FF) << 8);
  
  this->sockets[numSockets] = socket;
  this->numSockets++;
  
  this->sendTCP(socket, 0, 0, SYN);
  
  return socket;
}

TransmissionControlProtocolSocket* TransmissionControlProtocolProvider::listen(common::uint16_t port) {
  TransmissionControlProtocolSocket* socket = new TransmissionControlProtocolSocket(this);
  
  socket->state = LISTEN;
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
