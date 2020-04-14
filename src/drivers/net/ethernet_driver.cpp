#include <drivers/net/ethernet_driver.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;
using namespace sys::net;

void printf(char* str);

EthernetDriver::EthernetDriver()
:Driver()
{
  this->etherFrameProvider = 0;
  this->defaultMessage = "default EthernetDriver.\n";
}

EthernetDriver::~EthernetDriver() {}

void EthernetDriver::send(uint8_t* buffer, int size) {
  printf(this->defaultMessage);
}

void EthernetDriver::receive() {
  printf(this->defaultMessage);
}

uint64_t EthernetDriver::getMacAddress() {
  return 0;
}

uint32_t EthernetDriver::getIpAddress() {
  return 0;
}

void EthernetDriver::setIpAddress(uint32_t ip) {
  printf(this->defaultMessage);
}

bool EthernetDriver::hasLink() {
  return true; // Default
}

void EthernetDriver::setEtherFrameProvider(EtherFrameProvider* efp) {
  this->etherFrameProvider = efp;
}
