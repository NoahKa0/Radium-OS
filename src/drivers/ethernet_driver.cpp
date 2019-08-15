#include <drivers/ethernet_driver.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);

EthernetDriver::EthernetDriver()
:Driver()
{
  this->handler = 0;
  this->defaultMessage = "default EthernetDriver. This doesn't do anything!\n";
}

EthernetDriver::~EthernetDriver() {}

void EthernetDriver::send(uint8_t* buffer, int size) {
  printf(this->defaultMessage);
}

void EthernetDriver::receive() {
  printf(this->defaultMessage);
}

void EthernetDriver::setEtherFrameProvider(EtherFrameProvider* efp) {
  this->etherFrameProvider = efp;
}

uint64_t EthernetDriver::getMacAddress() {
  return 0;
}
