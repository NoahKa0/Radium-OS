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

bool TransmissionControlProtocolHandler::handleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, uint8_t* data, uint32_t length) {
  return true;
}

TransmissionControlProtocolSocket::TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend) {
  this->state = CLOSED;
  this->backend = backend;
  this->handler = 0;
}
TransmissionControlProtocolSocket::~TransmissionControlProtocolSocket() {
  // TODO clear buffers.
}

bool TransmissionControlProtocolSocket::handleTransmissionControlProtocolMessage(uint8_t* data, uint32_t length) {}

void TransmissionControlProtocolSocket::send(uint8_t* data, uint16_t length) {
  // TODO store data in buffer.
  // Actually, do this in sendTCP (because the header also needs to be stored), i just think that i will look here.
  // The buffer should also store the time when the header got created.
  // Also make a function that retransmits the packets when they didn't get acknowledged.
  this->backend->sendTCP(this, data, length, PSH);
}

void TransmissionControlProtocolSocket::setHandler(TransmissionControlProtocolHandler* handler) {
  this->handler = handler;
}

void TransmissionControlProtocolSocket::disconnect() {
  // TODO store time of disconnect, so the socket can get closed after waiting for FIN | ACK for too long.
  backend->disconnect(this);
}

bool TransmissionControlProtocolSocket::isClosed() {
  // TODO When time of disconnect is done, maybe this function should check that time.
  return this->state == CLOSED:
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

bool TransmissionControlProtocolProvider::onInternetProtocolReceived(uint32_t srcIp_BE, uint32_t destIp_BE, uint8_t* payload, uint32_t size) {
  if(size < 20) return false;
  
  TransmissionControlProtocolHeader* header = (TransmissionControlProtocolHeader*) payload;
  TransmissionControlProtocolSocket* socket = 0;
  
  for(uint16_t i = 0; i < numSockets && socket == 0; i++) {
    if(sockets[i]->localPort == header->destPort && sockets[i]->localIp == destIp_BE) {
      if(sockets[i]->state == LISTEN && (header->flags & (ACK | SYN | FIN)) == SYN) {
        socket = sockets[i];
      } else if(sockets[i]->remotePort == header->srcPort && sockets[i]->remoteIp == srcIp_BE) {
        socket = sockets[i];
      }
    }
  }
  
  if(socket != 0 && header->flags & RST) {
    socket->state = CLOSED;
  }
  
  if(socket != 0 && socket->state != CLOSED) {
    bool reset = false;
    bool remove = false;
    switch(header->flags & (ACK | SYN | FIN)) {
      case SYN:
        if(socket->state == LISTEN) {
          socket->remoteIp == srcIp_BE;
          socket->remotePort = header->srcPort;
          socket->state = SYN_RECEIVED;
          socket->acknowledgementNumber = bigEndian32(header->sequenceNumber) + 1;
          socket->sequenceNumber = this->generateSequenceNumber();
          this->sendTCP(socket, 0,0, SYN|ACK);
          socket->sequenceNumber++;
          return false; // Don't continue handling after acknowledgement is send.
        } else {
          reset = true;
        }
        break;
      case SYN | ACK:
        if(socket->state == SYN_SENT) {
          socket->state = ESTABLISHED;
          socket->acknowledgementNumber = bigEndian32(header->sequenceNumber) + 1;
          socket->sequenceNumber++;
          this->sendTCP(socket, 0,0, ACK);
        } else {
          reset = true;
        }
        break;
      case SYN | FIN:
      case SYN | FIN | ACK:
        reset = true;
        break;
      case FIN:
      case FIN | ACK:
        if(socket->state == ESTABLISHED) {
          socket->state = CLOSE_WAIT;
          socket->acknowledgementNumber++;
          this->sendTCP(socket, 0,0, ACK);
          this->sendTCP(socket, 0,0, FIN | ACK);
        } else if(socket->state == CLOSE_WAIT) {
          socket->state = CLOSED;
        } else if(socket->state == FIN_WAIT1 || socket->state == FIN_WAIT2) {
          socket->state = CLOSED;
          socket->acknowledgementNumber++;
          this->sendTCP(socket, 0,0, ACK);
        } else {
          reset = true;
        }
        break;
      case ACK:
        if(socket->state == SYN_RECEIVED) {
          socket->state = ESTABLISHED;
          break;
        } else if(socket->state == FIN_WAIT1) {
          socket->state = FIN_WAIT2;
          break;
        } else if(socket->state == CLOSE_WAIT) {
          socket->state = CLOSED;
          break;
        }
        // When connection is ESTABLISHED, an ACK might contain data.
        // NO BREAK.
      default:
        if(bigEndian32(header->sequenceNumber) == socket->acknowledgementNumber) {
          reset = !socket->handleTransmissionControlProtocolMessage(payload + (header->headerSize32*4), size - (header->headerSize32*4));
          if(!reset) {
            socket->acknowledgementNumber += size - (header->headerSize32*4);
            this->sendTCP(socket, 0,0, ACK);
          }
        } else {
          // Packets have arrived in different order...
          // It might be better to signal this to the sender... but the sender will eventually resend it, if it doesn't get the ACK.
          return false;
        }
        break;
    }
    if(reset) {
      // Can't handle resets for now, i don't think that would be a big problem though.
      return false;
    }
  }
  
  if(socket != 0 && socket->state == CLOSED) {
    bool move = false;
    for(uint16_t i = 0; i < numSockets; i++) {
      if(sockets[i] == socket) {
        move = true;
      }
      if(move) {
        this->sockets[i] = this->sockets[i+1];
      }
    }
    if(move) {
      this->numSockets--;
    }
  }
  
  return false;
}

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
  
  pseudoHeader->srcIp = socket->localIp;
  pseudoHeader->destIp = socket->remoteIp;
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
  socket->sequenceNumber = this->generateSequenceNumber();
  
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

uint32_t TransmissionControlProtocolProvider::generateSequenceNumber() {
  return 302342478; // This number should be randomly generated.
}
