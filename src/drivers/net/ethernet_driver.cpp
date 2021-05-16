#include <drivers/net/ethernet_driver.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;
using namespace sys::net;

EthernetDriver::EthernetDriver()
:Driver()
{
  this->etherFrameProvider = 0;
}

EthernetDriver::~EthernetDriver() {}

void EthernetDriver::send(uint8_t* buffer, int size) {}

void EthernetDriver::receive() {}

uint64_t EthernetDriver::getMacAddress() {
  return 0;
}

uint32_t EthernetDriver::getIpAddress() {
  return 0;
}

void EthernetDriver::setIpAddress(uint32_t ip) {}

bool EthernetDriver::hasLink() {
  return false;
}

void EthernetDriver::setEtherFrameProvider(EtherFrameProvider* efp) {
  this->etherFrameProvider = efp;
}
