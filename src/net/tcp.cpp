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

TransmissionControlProtocolSocket::TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend) {
  // Initialize send buffers.
  this->sendBufferSize = 128;
  uint8_t** sendBuffer = new uint8_t*[this->sendBufferSize]; // New array of uint8_t* returns a pointer to that array (so it's a pointer to a pointer),
  this->sendBufferPtr = (uint8_t*)sendBuffer;
  this->sendBufferPosition = 0;
  
  for(uint32_t i = 0; i < this->sendBufferSize; i++) {
    sendBuffer[i] = 0;
  }
  
  // Initialize receive buffers.
  this->recvBufferSize = 128;
  uint8_t** recvBuffer = new uint8_t*[this->recvBufferSize]; // New array of uint8_t* returns a pointer to that array (so it's a pointer to a pointer),
  this->recvBufferPtr = (uint8_t*)recvBuffer;
  this->recvBufferPosition = 0;
  this->recvBufferReadPacket = 0;
  this->recvBufferReadPosition = 0;
  this->nextBytes = 0;
  
  for(uint32_t i = 0; i < this->recvBufferSize; i++) {
    recvBuffer[i] = 0;
  }
  
  this->lastRecivedTime = SystemTimer::getTimeInInterrupts();
  
  this->state = CLOSED;
  this->backend = backend;
  
  this->packetSize = 1000;
}
TransmissionControlProtocolSocket::~TransmissionControlProtocolSocket() {
  this->backend->removeSocket(this);
  this->state = CLOSED;
  
  if(this->sendBufferPtr == 0) return;
  
  // Delete send buffers.
  uint8_t** sbuffer = (uint8_t**) this->sendBufferPtr;
  for(uint16_t i = 0; i < this->sendBufferSize; i++) {
    this->deleteSendPacket(i);
  }
  delete sbuffer;
  this->sendBufferPtr = 0;
  
  // Delete receive buffers.
  uint8_t** rbuffer = (uint8_t**) this->recvBufferPtr;
  for(uint16_t i = 0; i < this->recvBufferSize; i++) {
    this->deleteRecvPacket(i);
  }
  delete rbuffer;
  this->recvBufferPtr = 0;
}

uint32_t TransmissionControlProtocolSocket::hasNext() {
  return this->nextBytes;
}

void TransmissionControlProtocolSocket::readNext(uint8_t* data, uint32_t length) {
  if(length > this->nextBytes) {
    length = this->nextBytes;
  }
  
  uint32_t i = 0;
  while(i < length) {
    TransmissionControlProtocolPacket* packet = this->getRecvPacket(this->recvBufferReadPacket);
    TransmissionControlProtocolHeader* header = (TransmissionControlProtocolHeader*) (packet->data + sizeof(TransmissionControlProtocolPseudoHeader));
    uint8_t* pdata = packet->data + sizeof(TransmissionControlProtocolPseudoHeader) + (header->headerSize32*4);
    uint32_t packetLength = packet->length - (header->headerSize32*4);
    if(packetLength-this->recvBufferReadPosition > (length-i)) {
      uint16_t read = length-i;
      for(uint32_t j = this->recvBufferReadPosition; j < read; i++) {
        data[i] = pdata[j];
        i++;
      }
      this->recvBufferReadPosition += read;
    } else {
      for(uint32_t j = this->recvBufferReadPosition; j < packetLength; j++) {
        data[i] = pdata[j];
        i++;
      }
      this->deleteRecvPacket(this->recvBufferReadPacket);
      this->recvBufferReadPosition = 0;
      this->recvBufferReadPacket = (this->recvBufferReadPacket+1)%this->recvBufferSize;
    }
  }
  this->nextBytes -= length;
}

void TransmissionControlProtocolSocket::removeOldPackets(uint32_t acknum) {
  for(uint32_t i = 0; i < this->sendBufferSize; i++) {
    TransmissionControlProtocolPacket* packet = this->getSendPacket(i);
    // Check to see if packet sequenceNumber is acked.
    // The second part is to detect when the ack number wraps around
    if(packet != 0
      && (packet->sequenceNumber < acknum || ((packet->sequenceNumber & 0xF0000000) != 0 && (acknum & 0xFF000000) == 0)))
    {
      this->deleteSendPacket(i);
    }
  }
}

TransmissionControlProtocolPacket* TransmissionControlProtocolSocket::getRecvPacket(common::uint16_t n) {
  if(this->recvBufferPtr == 0 || n >= this->recvBufferSize) return 0;
  
  uint8_t** buffer = (uint8_t**) this->recvBufferPtr;
  return (TransmissionControlProtocolPacket*) buffer[n];
}

TransmissionControlProtocolPacket* TransmissionControlProtocolSocket::getSendPacket(common::uint16_t n) {
  if(this->sendBufferPtr == 0 || n >= this->sendBufferSize) return 0;
  
  uint8_t** buffer = (uint8_t**) this->sendBufferPtr;
  return (TransmissionControlProtocolPacket*) buffer[n];
}

bool TransmissionControlProtocolSocket::addRecvPacket(TransmissionControlProtocolPacket* packet, uint32_t dataLength) {
  uint8_t** buffer = (uint8_t**) this->recvBufferPtr;
  if(buffer[this->recvBufferPosition] != 0) {
    return false;
  }
  buffer[this->recvBufferPosition] = (uint8_t*) packet;
  this->recvBufferPosition = (this->recvBufferPosition+1)%this->recvBufferSize;
  this->nextBytes += dataLength;
  return true;
}

bool TransmissionControlProtocolSocket::addSendPacket(TransmissionControlProtocolPacket* packet) {
  uint8_t** buffer = (uint8_t**) this->sendBufferPtr;
  if(buffer[this->sendBufferPosition] != 0) {
    return false;
  }
  buffer[this->sendBufferPosition] = (uint8_t*) packet;
  this->sendBufferPosition = (this->sendBufferPosition+1)%this->sendBufferSize;
  return true;
}

void TransmissionControlProtocolSocket::deleteRecvPacket(common::uint16_t n) {
  if(this->recvBufferPtr == 0 || n >= this->recvBufferSize) return;
  
  uint8_t** buffer = (uint8_t**) this->recvBufferPtr;
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

void TransmissionControlProtocolSocket::deleteSendPacket(common::uint16_t n) {
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

void TransmissionControlProtocolSocket::sendExpiredPackets() {
  this->backend->sendExpiredPackets(this);
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
          socket->removeOldPackets(socket->sequenceNumber);
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
        // When connection is ESTABLISHED, an ACK might contain data.
        // NO BREAK.
      default:
        socket->removeOldPackets(socket->sequenceNumber);
        if(bigEndian32(header->sequenceNumber) == socket->acknowledgementNumber && size - (header->headerSize32*4) > 0) {
          // Save checksum
          uint16_t recvChecksum = header->checksum;
          header->checksum = 0; // Checksum should be 0 when recalculating.
          
          uint8_t* recvPacketPtr = (uint8_t*) MemoryManager::activeMemoryManager->malloc(size + sizeof(TransmissionControlProtocolPseudoHeader));
          uint8_t* recvHeaderPtr = recvPacketPtr + sizeof(TransmissionControlProtocolPseudoHeader);
          for(uint32_t i = 0; i < size; i++) {
            recvHeaderPtr[i] = payload[i];
          }
          
          // Reconstruct pseudoHeader.
          TransmissionControlProtocolPseudoHeader* pseudoHeader = (TransmissionControlProtocolPseudoHeader*) recvPacketPtr;
          pseudoHeader->srcIp = socket->remoteIp;
          pseudoHeader->destIp = socket->localIp;
          pseudoHeader->protocol = 0x0600; // BE 6.
          pseudoHeader->totalLength = ((size & 0x00FF) << 8) | ((size & 0xFF00) >> 8);
          
          // Recalculate checksum.
          uint16_t newChecksum = InternetProtocolV4Provider::checksum((uint16_t*) recvPacketPtr, size + sizeof(TransmissionControlProtocolPseudoHeader));
          
          if(newChecksum == recvChecksum) {
            uint8_t* packetPtr = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(TransmissionControlProtocolPacket));
            TransmissionControlProtocolPacket* packet = (TransmissionControlProtocolPacket*) packetPtr;
            packet->lastTransmit = 0; // Doesn't matter.
            packet->length = size;
            packet->sequenceNumber = 0; // Doesn't matter.
            // recvPacketPtr will get freed when packet gets deleted from socket.
            packet->data = recvPacketPtr;
            
            // If packet is added to buffer, acknowledge packet
            if(socket->addRecvPacket(packet, size - (header->headerSize32*4))) {
              socket->acknowledgementNumber += size - (header->headerSize32*4);
            }
          } else {
            MemoryManager::activeMemoryManager->free(recvPacketPtr); // Packet not added, delete recvPacketPtr
          }
        }
        if(!reset && size - (header->headerSize32*4) > 0) {
          this->sendTCP(socket, 0,0, ACK, true);
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
    while(!socket->addSendPacket(packet)) {
      this->sendExpiredPackets(socket);
    }
    this->sendExpiredPackets(socket);
  }
}

void TransmissionControlProtocolProvider::sendExpiredPackets(TransmissionControlProtocolSocket* socket) {
  for(uint32_t i = 0; i < socket->sendBufferSize; i++) {
    TransmissionControlProtocolPacket* packet = socket->getSendPacket(i);
    // If lastTransmit was more than 18 PIT interrupts ago (that's about one second), resend.
    if(packet != 0 && packet->lastTransmit + 9 < SystemTimer::getTimeInInterrupts()) {
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
