#include <drivers/net/broadcom_BCM5751.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);
void setSelectedEthernetDriver(EthernetDriver* drv);

broadcom_BCM5751::broadcom_BCM5751(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager)
: EthernetDriver(),
InterruptHandler(device->interrupt + 0x20, interruptManager), // hardware interrupt is asigned, so the device doesn't know that it starts at 0x20.
{
  // TODO
  
  setSelectedEthernetDriver(this); // Make this instance accessable in kernel.cpp
}

broadcom_BCM5751::~broadcom_BCM5751() {}

void broadcom_BCM5751::activate() {
  // TODO
}

int broadcom_BCM5751::reset() {
  // TODO
  return 0;
}

common::uint32_t broadcom_BCM5751::handleInterrupt(common::uint32_t esp) {
  // TODO
  return esp;
}

void amd_am79c973::send(common::uint8_t* buffer, int size) {
  // TODO
}

void amd_am79c973::receive() {
  // TODO
}

uint64_t amd_am79c973::getMacAddress() {
  // TODO
  return 0;
}

uint32_t amd_am79c973::getIpAddress() {
  // TODO
  return 0;
}

void amd_am79c973::setIpAddress(uint32_t ip) {
  // TODO
}
