#include <net/etherframe.h>
#include <memorymanagement.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::drivers;

EtherFrameHandler::EtherFrameHandler(EtherFrameProvider* backend, common::uint16_t etherType) {
  this->backend = backend;
  this->etherType = ((etherType & 0x00FF) << 8) | (etherType & 0xFF00) >> 8);
  backend->handlers[etherType] = this;
}

EtherFrameHandler::~EtherFrameHandler() {
  backend->handlers[etherType] = 0;
}

bool EtherFrameHandler::onEtherFrameReceived(uint8_t* etherFramePayload, uint32_t size) {
  return false;
}

void EtherFrameHandler::send(uint64_t destMacAddress, uint8_t* etherFramePayload, uint32_t size) {
  backend->send(destMacAddress, etherType, etherFramePayload, size);
}


EtherFrameProvider::EtherFrameProvider(EthernetDriver* backend) {
  this->ethernetDriver = backend;
  for(uint32_t i = 0; i < 65535; i++) {
    handlers[i] = 0;
  }
}

EtherFrameProvider::~EtherFrameProvider() {}

bool EtherFrameProvider::onRawDataRecived(uint8_t* buffer, uint32_t size) {
  EtherFrameHeader* header = (EtherFrameHeader*) buffer;
  bool sendBack = false;
  
  if(header->destMac == 0xFFFFFFFFFFFF || // Broadcast
    header->destMac == this->ethernetDriver->getMacAddress())
  {
    if(handlers[header->etherType] != 0) {
      sendBack = handlers[header->etherType]->onEtherFrameReceived(buffer + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
    }
  }
  
  if(sendBack) {
    header->destMac = header->srcMac;
    header->srcMac = this->ethernetDriver->getMacAddress();
  }
  
  return sendBack;
}

void EtherFrameProvider::send(uint64_t destMacAddress, uint16_t etherType, uint8_t* buffer, uint32_t size) {
  uint8_t* buffer2 = MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
  EtherFrameHeader* header = (EtherFrameHeader*) buffer2;
  
  header->destMac = destMacAddress;
  header->srcMac = this->ethernetDriver->getMacAddress();
  header->etherType = this->etherType;
  
  uint8_t* dst = buffer2 + sizeof(EtherFrameHeader);
  for(uint32_t i = 0; i < size; i++) {
    dst[i] = buffer[i];
  }
  
  this->ethernetDriver->send(buffer, size + sizeof(EtherFrameHeader));
  MemoryManager::activeMemoryManager->free(buffer2);
}
