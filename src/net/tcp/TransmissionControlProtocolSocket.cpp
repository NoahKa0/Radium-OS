#include <net/tcp/TransmissionControlProtocolSocket.h>
#include <net/tcp/TransmissionControlProtocolProvider.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::net::tcp;

uint32_t swapEndian32(uint32_t n);

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

  this->sizeUnprocessedPackets = 128;
  this->unprocessedPackets = (TransmissionControlProtocolPacket**) MemoryManager::activeMemoryManager->malloc(sizeof(TransmissionControlProtocolPacket*) * this->sizeUnprocessedPackets);
  for(uint32_t i = 0; i < this->sizeUnprocessedPackets; i++) {
    this->unprocessedPackets[i] = 0;
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

  for(uint8_t i = 0; i < this->sizeUnprocessedPackets; i++) {
    if(this->unprocessedPackets[i] != 0) {
      delete this->unprocessedPackets[i]->data;
      delete this->unprocessedPackets[i];
      this->unprocessedPackets[i] = 0;
    }
  }
  delete this->unprocessedPackets;
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
      for(uint32_t j = 0; j < read; j++) {
        data[i] = pdata[this->recvBufferReadPosition + j];
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
      && (packet->sequenceNumber <= acknum || ((packet->sequenceNumber & 0xF0000000) != 0 && (acknum & 0xFF000000) == 0)))
    {
      this->deleteSendPacket(i);
    }
  }
}

bool TransmissionControlProtocolSocket::addUnprocessedPacket(TransmissionControlProtocolPacket* packet) {
  for(uint8_t i = 0; i < this->sizeUnprocessedPackets; i++) {
    if(this->unprocessedPackets[i] == 0) {
      this->unprocessedPackets[i] = packet;
      return true;
    } else if(swapEndian32(this->unprocessedPackets[i]->sequenceNumber) < this->acknowledgementNumber && this->acknowledgementNumber & 0xF0000000 == 0) {
      delete this->unprocessedPackets[i]->data;
      delete this->unprocessedPackets[i];
      this->unprocessedPackets[i] = 0;
    }
  }
  delete this->unprocessedPackets[this->sizeUnprocessedPackets-1]->data;
  delete this->unprocessedPackets[this->sizeUnprocessedPackets-1];
  this->unprocessedPackets[this->sizeUnprocessedPackets-1] = packet;
  return true;
}
TransmissionControlProtocolPacket* TransmissionControlProtocolSocket::getUnprocessedPacket() {
  for(uint8_t i = 0; i < this->sizeUnprocessedPackets; i++) {
    if(this->unprocessedPackets[i] != 0) {
      if(swapEndian32(this->unprocessedPackets[i]->sequenceNumber) == this->acknowledgementNumber) {
        TransmissionControlProtocolPacket* ret = this->unprocessedPackets[i];
        this->unprocessedPackets[i] = 0;
        return ret;
      }

      if(swapEndian32(this->unprocessedPackets[i]->sequenceNumber) < this->acknowledgementNumber && this->acknowledgementNumber & 0xF0000000 == 0) {
        delete this->unprocessedPackets[i]->data;
        delete this->unprocessedPackets[i];
        this->unprocessedPackets[i] = 0;
      }
    }
  }
  return 0;
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
