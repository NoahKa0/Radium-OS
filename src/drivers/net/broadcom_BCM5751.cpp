#include <drivers/net/broadcom_BCM5751.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);
void setSelectedEthernetDriver(EthernetDriver* drv);

#define csr32(c, r)	((c)[(r)/4])

broadcom_BCM5751::broadcom_BCM5751(PeripheralComponentDeviceDescriptor* device, BaseAddressRegister* bar, InterruptManager* interruptManager)
: EthernetDriver(),
InterruptHandler(device->interrupt + 0x20, interruptManager) // hardware interrupt is asigned, so the device doesn't know that it starts at 0x20.
{
  // TODO initialize ctlr
  
  uint16_t* nic = 0; // TODO use nic from ctlr
  uint16_t* mem = nic + 0x8000;
  
	csr32(nic, MiscHostCtl) |= MaskPCIInt | ClearIntA;
	csr32(nic, SwArbitration) |= SwArbitSet1;
	while((csr32(nic, SwArbitration) & SwArbitWon1) == 0);
  
	csr32(nic, MemArbiterMode) |= Enable;
	csr32(nic, MiscHostCtl) |= IndirectAccessEnable | EnablePCIStateRegister | EnableClockControlRegister;
	csr32(nic, MemoryWindow) = 0;
	csr32(mem, 0xB50) = 0x4B657654; // magic number
	
  csr32(nic, MiscConfiguration) |= GPHYPowerDownOverride | DisableGRCResetOnPCIE;
	csr32(nic, MiscConfiguration) |= CoreClockBlocksReset;
	SystemTimer::sleep(150); // I should wait 100 ms, but the timer isn't that accurate, so wait slightly more.
  
  
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

void broadcom_BCM5751::send(common::uint8_t* buffer, int size) {
  // TODO
}

void broadcom_BCM5751::receive() {
  // TODO
}

uint64_t broadcom_BCM5751::getMacAddress() {
  // TODO
  return 0;
}

uint32_t broadcom_BCM5751::getIpAddress() {
  // TODO
  return 0;
}

void broadcom_BCM5751::setIpAddress(uint32_t ip) {
  // TODO
}
