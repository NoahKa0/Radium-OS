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

void TransmissionControlProtocolProvider::sendTCP(TransmissionControlProtocolSocket* socket, uint8_t* data, uint16_t length, uint16_t flags) {
  uint16_t packetLength = length + sizeof(TransmissionControlProtocolHeader);
  uint16_t totalLength = packetLength + sizeof(TransmissionControlProtocolPseudoHeader);
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
  
  header->checksum = 0;
  
  pseudoHeader->srcIP = socket->localIp;
  pseudoHeader->destIP = socket->remoteIp;
  pseudoHeader->protocol = 0x0600; // BE 6.
  
  
  
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
