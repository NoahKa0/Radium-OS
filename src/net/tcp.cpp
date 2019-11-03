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
  // Initialize sendBuffers (buffers that store packets in case they get lost).
  this->sendBufferSize = 16;
  uint8_t** sendBuffer = new uint8_t*[this->sendBufferSize]; // New array of uint8_t* returns a pointer to that array (so it's a pointer to a pointer),
  this->sendBufferPtr = (uint8_t*)sendBuffer;
  this->currentBufferPosition = 0;
  
  for(uint32_t i = 0; i < this->sendBufferSize; i++) {
    sendBuffer[i] = 0;
  }
  
  this->lastRecivedTime = SystemTimer::getTimeInInterrupts();
  
  this->state = CLOSED;
  this->backend = backend;
  this->handler = 0;
  
  this->packetSize = 1000;
}
TransmissionControlProtocolSocket::~TransmissionControlProtocolSocket() {
  this->backend->removeSocket(this);
  this->state = CLOSED;
  
  if(this->sendBufferPtr == 0) return;
  
  // We need to delete all packets that this socket has buffered.
  uint8_t** buffer = (uint8_t**) this->sendBufferPtr;
  for(uint16_t i = 0; i < this->sendBufferSize; i++) {
    this->deletePacket(i);
  }
  delete buffer;
  this->sendBufferPtr = 0;
}

void TransmissionControlProtocolSocket::removeOldPackets(uint32_t acknum) {
  for(uint32_t i = 0; i < this->sendBufferSize; i++) {
    TransmissionControlProtocolPacket* packet = this->getPacket(i);
    // Check to see if packet sequenceNumber is acked.
    // The second part is to detect when the ack number wraps around
    if(packet != 0
      && (packet->sequenceNumber <= acknum || ((packet->sequenceNumber & 0xC0000000) == 0xC0000000 && (acknum & 0x80000000) != 0x80000000)))
    {
      this->deletePacket(i);
    }
  }
}

TransmissionControlProtocolPacket* TransmissionControlProtocolSocket::getPacket(common::uint16_t n) {
  if(this->sendBufferPtr == 0 || n >= this->sendBufferSize) return 0;
  
  uint8_t** buffer = (uint8_t**) this->sendBufferPtr;
  return (TransmissionControlProtocolPacket*) buffer[n];
}

bool TransmissionControlProtocolSocket::addPacket(TransmissionControlProtocolPacket* packet) {
  uint8_t** buffer = (uint8_t**) this->sendBufferPtr;
  if(buffer[this->currentBufferPosition] != 0) {
    return false;
  }
  buffer[this->currentBufferPosition] = (uint8_t*) packet;
  this->currentBufferPosition = (this->currentBufferPosition+1)%this->sendBufferSize;
  return true;
}

void TransmissionControlProtocolSocket::deletePacket(common::uint16_t n) {
  if(this->sendBufferPtr == 0 || n >= this->sendBufferSize) return;
  
  uint8_t** buffer = (uint8_t**) this->sendBufferPtr;
  if(buffer[n] != 0) {
    uint8_t* bufferData = buffer[n];
    TransmissionControlProtocolPacket* packet = (TransmissionControlProtocolPacket*) bufferData;
    if(packet->data != 0) {
      delete packet->data; // Packet data is a pointer so it should also be deleted.
      packet->data = 0;
    }
    delete packet;
    buffer[n] = 0;
  }
}

bool TransmissionControlProtocolSocket::handleTransmissionControlProtocolMessage(uint8_t* data, uint32_t length) {
  if(this->handler != 0) {
    return this->handler->handleTransmissionControlProtocolMessage(this, data, length);
  }
  return true;
}

void TransmissionControlProtocolSocket::send(uint8_t* data, uint32_t length) {
  uint32_t packetSize = this->packetSize;
  uint32_t bytesToSend = length;
  while(bytesToSend > 0) {
    if(bytesToSend > packetSize) {
      this->backend->sendTCP(this, data, packetSize, ACK);
      bytesToSend -= packetSize;
      data += packetSize;
    } else {
      this->backend->sendTCP(this, data, bytesToSend, PSH | ACK);
      bytesToSend = 0;
    }
  }
}

void TransmissionControlProtocolSocket::setHandler(TransmissionControlProtocolHandler* handler) {
  this->handler = handler;
}

void TransmissionControlProtocolSocket::disconnect() {
  this->lastRecivedTime = SystemTimer::getTimeInInterrupts();
  backend->disconnect(this);
}

bool TransmissionControlProtocolSocket::isConnected() {
  // TODO Check lastRecivedTime
  return this->state == ESTABLISHED;
}

bool TransmissionControlProtocolSocket::isClosed() {
  // TODO Check lastRecivedTime
  return this->state == CLOSED;
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
  
  if(socket != 0 && (header->flags & RST) == RST) {
    socket->state = CLOSED;
  }
  
  if(socket != 0 && socket->state != CLOSED) {
    socket->lastRecivedTime = SystemTimer::getTimeInInterrupts();
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
          this->sendTCP(socket, 0,0, SYN|ACK, true);
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
          this->sendTCP(socket, 0,0, ACK, true);
          socket->removeOldPackets(header->acknowledgementNumber);
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
          this->sendTCP(socket, 0,0, ACK, true);
          this->sendTCP(socket, 0,0, FIN | ACK, true);
        } else if(socket->state == CLOSE_WAIT) {
          socket->state = CLOSED;
        } else if(socket->state == FIN_WAIT1 || socket->state == FIN_WAIT2) {
          socket->state = CLOSED;
          socket->acknowledgementNumber++;
          this->sendTCP(socket, 0,0, ACK, true);
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
        socket->removeOldPackets(header->acknowledgementNumber);
        // When connection is ESTABLISHED, an ACK might contain data.
        // NO BREAK.
      default:
        if(bigEndian32(header->sequenceNumber) == socket->acknowledgementNumber) {
          reset = !socket->handleTransmissionControlProtocolMessage(payload + (header->headerSize32*4), size - (header->headerSize32*4));
          
          if(!reset && bigEndian32(header->sequenceNumber) <= socket->acknowledgementNumber) {
            socket->acknowledgementNumber += size - (header->headerSize32*4);
            this->sendTCP(socket, 0,0, ACK, true);
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
    this->removeSocket(socket);
  }
  
  return false;
}

void TransmissionControlProtocolProvider::sendTCP(TransmissionControlProtocolSocket* socket, uint8_t* data, uint16_t length, uint16_t flags, bool noRetransmit) {
  // Length without pseudoHeader.
  uint16_t packetLength = length + sizeof(TransmissionControlProtocolHeader);
  
  // Length including pseudoHeader.
  uint16_t totalLength = length + sizeof(TransmissionControlProtocolHeader) + sizeof(TransmissionControlProtocolPseudoHeader);
  // Ptr to full packet.
  uint8_t* buffer = (uint8_t*) MemoryManager::activeMemoryManager->malloc(totalLength);
  
  // pseudoHeader needed for checksum.
  TransmissionControlProtocolPseudoHeader* pseudoHeader = (TransmissionControlProtocolPseudoHeader*) buffer;
  // Ptr to header (inside buffer).
  TransmissionControlProtocolHeader* header = (TransmissionControlProtocolHeader*) (buffer + sizeof(TransmissionControlProtocolPseudoHeader));
  // Ptr to payload (inside buffer).
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
  
  // Copy data to payload.
  for(int i = 0; i < length; i++) {
    payload[i] = data[i];
  }
  
  // Set pseudoHeader values
  pseudoHeader->srcIp = socket->localIp;
  pseudoHeader->destIp = socket->remoteIp;
  pseudoHeader->protocol = 0x0600; // BE 6.
  pseudoHeader->totalLength = ((packetLength & 0x00FF) << 8) | ((packetLength & 0xFF00) >> 8);
  
  // Calculate checksum (assuming checksum of 0).
  header->checksum = 0;
  header->checksum = InternetProtocolV4Provider::checksum((uint16_t*) buffer, totalLength);
  
  // Create a new packet to que inside socket.
  uint8_t* packetPtr = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(TransmissionControlProtocolPacket));
  TransmissionControlProtocolPacket* packet = (TransmissionControlProtocolPacket*) packetPtr;
  
  if(noRetransmit) {
    // Just transmit without storing, this should only happen for acks.
    this->send(socket->remoteIp, (uint8_t*) header, packetLength);
    delete buffer;
  } else {
    // Last transmission time, used for retransmission when no ack arrives.
    packet->lastTransmit = 0;
    packet->length = packetLength;
    packet->sequenceNumber = socket->sequenceNumber;
    // buffer will get freed when packet gets deleted from socket.
    packet->data = buffer;
    
    // Que packet.
    while(!socket->addPacket(packet)) {
      this->sendExpiredPackets(socket);
    }
    this->sendExpiredPackets(socket);
  }
}

void TransmissionControlProtocolProvider::sendExpiredPackets(TransmissionControlProtocolSocket* socket) {
  for(uint32_t i = 0; i < socket->sendBufferSize; i++) {
    TransmissionControlProtocolPacket* packet = socket->getPacket(i);
    // If lastTransmit was more than 18 PIT interrupts ago (that's about one second), resend.
    if(packet != 0 && packet->lastTransmit + 18 < SystemTimer::getTimeInInterrupts()) {
      uint8_t* headerPtr = packet->data + sizeof(TransmissionControlProtocolPseudoHeader);
      
      this->send(socket->remoteIp, headerPtr, packet->length);
      packet->lastTransmit = SystemTimer::getTimeInInterrupts();
    }
  }
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

void TransmissionControlProtocolProvider::removeSocket(TransmissionControlProtocolSocket* socket) {
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

uint32_t TransmissionControlProtocolProvider::generateSequenceNumber() {
  return 302342478; // This number should be randomly generated.
}
