#include <net/ipv4.h>
#include <memorymanagement.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;

InternetProtocolV4Handler::InternetProtocolV4Handler(InternetProtocolV4Provider* backend, uint8_t ipProtocol) {
  this->ipProtocol = ipProtocol;
  this->backend = backend;
  backend->handlers[ipProtocol] = this;
}

InternetProtocolV4Handler::~InternetProtocolV4Handler() {
  if(backend->handlers[ipProtocol] == this) {
    backend->handlers[ipProtocol] = 0;
  }
}

bool InternetProtocolV4Handler::onInternetProtocolReceived(uint32_t srcIp_BE, uint32_t destIp_BE, uint8_t* payload, uint32_t size) {
  return false;
}

void InternetProtocolV4Handler::send(uint32_t destIp_BE, uint8_t* payload, uint32_t size) {
  backend->send(destIp_BE, this->ipProtocol, payload, size);
}


InternetProtocolV4Provider::InternetProtocolV4Provider(EtherFrameProvider* backend, AddressResolutionProtocol* arp,
  uint32_t gateway, uint32_t subnetMask)
  : EtherFrameHandler(backend, 0x800)
{
  for(int i = 0; i < 255; i++) {
    handlers[i] = 0;
  }
  this->arp = arp;
  this->gateway = gateway;
  this->subnetMask = subnetMask;
}
  
InternetProtocolV4Provider::~InternetProtocolV4Provider() {}

bool InternetProtocolV4Provider::onEtherFrameReceived(uint8_t* etherFramePayload, uint32_t size) {
  if(size < sizeof(InternetProtocolV4Message)) return false;
  
  InternetProtocolV4Message* ipMessage = (InternetProtocolV4Message*) etherFramePayload;
  bool sendBack = false;
  if(ipMessage->destAddress == backend->getIpAddress()) { // Only handle messages directed to us.
    uint32_t messageLength = ipMessage->length;
    if(messageLength > size) { // Size of message can't be larger than size of EtherFrame.
      messageLength = size;
    }
    if(handlers[ipMessage->protocol] != 0) {
      sendBack = handlers[ipMessage->protocol]->onInternetProtocolReceived(
              ipMessage->srcAddress,
              ipMessage->destAddress,
              etherFramePayload + (ipMessage->headerLength*4),
              messageLength - (ipMessage->headerLength*4));
    }
  }
  
  if(sendBack) {
    ipMessage->destAddress = ipMessage->srcAddress;
    ipMessage->srcAddress = backend->getIpAddress();
    
    ipMessage->timeToLive = 0x40;
    
    ipMessage->checksum = 0; // Checksum should be 0 before checking
    ipMessage->checksum = checksum((uint16_t*) ipMessage, 4*ipMessage->headerLength);
  }
  
  return sendBack;
}

void InternetProtocolV4Provider::send(uint32_t destIpAddress, uint8_t protocol, uint8_t* etherFramePayload, uint32_t size) {
  uint8_t* buffer = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(InternetProtocolV4Message) + size);
  InternetProtocolV4Message* message = (InternetProtocolV4Message*) buffer;
  
  message->version = 4;
  message->headerLength = sizeof(InternetProtocolV4Message)/4;
  message->tos = 0;
  
  message->length = size + sizeof(InternetProtocolV4Message);
  message->length = ((message->length & 0x00FF) << 8) | ((message->length & 0xFF00) >> 8); // to BE.
  
  message->identification = 0x0100;
  
  // first bit is reserved. Second bit is don't fragment. Third bit is more fragments.
  // 0x02 = 0d010, so Don't fragment is set.
  // I'm not sure whether More Fragments = 0d100 or 0d001, but we don't need it anyway.
  message->flags = 0x02;
  
  message->fragmentOffset = 0x0000;
  message->timeToLive = 0x40;
  message->protocol = protocol;
  message->destAddress = destIpAddress;
  message->srcAddress = backend->getIpAddress();
  
  message->checksum = 0; // Checksum also uses this value to calculate full checksum. Package will drop if not 0.
  message->checksum = checksum((uint16_t*) message, sizeof(InternetProtocolV4Message));
  
  uint8_t* dataTarget = buffer + sizeof(InternetProtocolV4Message);
  for(uint32_t i = 0; i < size; i++) {
    dataTarget[i] = etherFramePayload[i];
  }
  
  uint32_t route = destIpAddress;
  if((destIpAddress & subnetMask) != (message->srcAddress & subnetMask)) {
    route = gateway;
  }
  
  backend->send(arp->resolve(route), this->etherType, buffer, sizeof(InternetProtocolV4Message) + size);
  
  MemoryManager::activeMemoryManager->free(buffer);
}

uint16_t InternetProtocolV4Provider::checksum(uint16_t* data, uint32_t size) {
  uint32_t tmp = 0;
  uint32_t loop = size/2;
  for(uint32_t i = 0; i < loop; i++) {
    tmp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);
  }
  if(size % 2) {
    tmp += ((uint16_t) ((char*) data)[size-1]) << 8;
  }
  
  while(tmp & 0xFFFF0000) {
    tmp = (tmp & 0xFFFF) + (tmp >> 16);
  }
  
  return ((~tmp & 0xFF00) >> 8) | ((~tmp & 0x00FF) << 8);
}
