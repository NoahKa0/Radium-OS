#include <net/etherframe.h>
#include <memorymanagement.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::drivers;

AddressResolutionProtocol::AddressResolutionProtocol(EtherFrameProvider* backend)
: EtherFrameHandler(backend, 0x806)
{
  this->currentCache = 0;
}

AddressResolutionProtocol::~AddressResolutionProtocol();

bool AddressResolutionProtocol::onEtherFrameReceived(uint8_t* etherFramePayload, uint32_t size) {
  if(size < sizeof(AddressResolutionProtocolMessage)) return false;
  
  AddressResolutionProtocolMessage* arp = (AddressResolutionProtocolMessage*) etherFramePayload;
  
  if(arp->hardwareType == 0x0100) {
    if(arp->protocol == 0x0008
    && arp->hardwareAddressLength == 6
    && arp->protocolAddressLength == 4
    && arp->destIpAddress == backend->getIpAddress()) {
      switch(arp->command) {
        case 0x0100:
          arp->command = 0x0200;
          arp->destIpAddress = arp->senderIpAddress;
          arp->destMacAddress = arp->senderMacAddress;
          arp->senderIpAddress = backend->getIpAddress();
          arp->senderMacAddress = backend->getMacAddress();
          return true;
          break;
        case 0x0200:
          ipCache[currentCache] = arp->senderIpAddress;
          macCache[currentCache] = arp->senderMacAddress;
          currentCache = (currentCache+1)%128;
        default:
          break;
      }
    }
  }
  
  return false;
}

void AddressResolutionProtocol::requestMacAddress(uint32_t ip_BE) {
  AddressResolutionProtocolMessage arp;
  arp.hardwareType = 0x0100; // BE 1.
  arp.protocol = 0x0008; // BE 800. 800 = IPv4.
  arp.hardwareAddressLength = 6; // MAC Address = 6 bytes.
  arp.protocolAddressLength = 4; // IPv4 address = 4 bytes.
  arp.command = 0x0100; // BE 1. 1 = request.
  arp.senderMacAddress = backend->getMacAddress();
  arp.senderIpAddress = backend->getIpAddress();
  arp.destMacAddress = 0xFFFFFFFFFFFF;
  arp.destIpAddress = ip_BE;
  
  this->send(arp.destMacAddress, &arp, sizeof(AddressResolutionProtocolMessage));
}

uint64_t AddressResolutionProtocol::getMacFromCache(uint32_t ip_BE) {
  for(int i = 0; i < 128; i++) {
    if(ipCache[i] == ip_BE) {
      return macCache[i];
    }
  }
  return 0xFFFFFFFFFFFF;
}

uint64_t resolve(uint32_t ip_BE) {
  uint64_t result = getMacFromCache(ip_BE);
  uint32_t timeout = 0;
  
  if(result == 0xFFFFFFFFFFFF) {
    requestMacAddress(ip_BE);
  }
  
  while(result == 0xFFFFFFFFFFFF && timeout < 799) {
    asm("hlt");
    timeout++;
    result = getMacFromCache(ip_BE);
    if(timeout%200 == 0 && result == 0xFFFFFFFFFFFF) {
      requestMacAddress(ip_BE);
    }
  }
}
