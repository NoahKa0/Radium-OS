#include <drivers/amd_am79c973.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);

amd_am79c973::amd_am79c973(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager)
: Driver(),
InterruptHandler(device->interrupt + 0x20, interruptManager), // hardware interrupt is asigned, so the device doesn't know that it starts at 0x20.
macAddress0Port(device->portBase),
macAddress2Port(device->portBase + 0x02),
macAddress4Port(device->portBase + 0x04),
registerDataPort(device->portBase + 0x10),
registerAddressPort(device->portBase + 0x12),
resetPort(device->portBase + 0x14),
busControlRegisterDataPort(device->portBase + 0x16)
{
  currentSendBuffer = 0;
  currentReciveBuffer = 0;
  
  uint64_t MAC0 = macAddress0Port.read() % 256;
  uint64_t MAC1 = macAddress0Port.read() / 256;
  uint64_t MAC2 = macAddress2Port.read() % 256;
  uint64_t MAC3 = macAddress2Port.read() / 256;
  uint64_t MAC4 = macAddress4Port.read() % 256;
  uint64_t MAC5 = macAddress4Port.read() / 256;
  
  uint64_t MAC = MAC5 << 40
               | MAC4 << 32
               | MAC3 << 24
               | MAC2 << 16
               | MAC1 << 8
               | MAC0;
  
  // 32 Bit mode?
  registerAddressPort.write(20);
  busControlRegisterDataPort.write(0x102);
  
  // Reset
  registerAddressPort.write(0);
  registerDataPort.write(0x04);
  
  initBlock.mode = 0x0000;
  
  initBlock.reserved1 = 0;
  initBlock.numSendBuffers = 3;
  
  initBlock.reserved2 = 0;
  initBlock.numReciveBuffers = 3;
  initBlock.physicalAddress = MAC;
  
  initBlock.reserved3 = 0;
  initBlock.logicalAddress = 0;
  
  sendBufferDescr = (BufferDescriptor*)((((uint32_t)&sendBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
  initBlock.sendBufferDescrAddress = (uint32_t)sendBufferDescr;
  
  reciveBufferDescr = (BufferDescriptor*)((((uint32_t)&reciveBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
  initBlock.reciveBufferDescrAddress = (uint32_t)reciveBufferDescr;
  
  for(uint8_t i = 0; i < 8; i++) {
    sendBufferDescr[i].address = (((uint32_t)&sendBuffers[i]) + 15 ) & ~(uint32_t)0xF;
    sendBufferDescr[i].flags = 0x7FF
                             | 0xF000;
    sendBufferDescr[i].flags2 = 0;
    sendBufferDescr[i].available = 0;

    reciveBufferDescr[i].address = (((uint32_t)&reciveBuffers[i]) + 15 ) & ~(uint32_t)0xF;
    reciveBufferDescr[i].flags = 0xF7FF
                             | 0x80000000;
    reciveBufferDescr[i].flags2 = 0;
    sendBufferDescr[i].available = 0;
  }
  
  registerAddressPort.write(1);
  registerDataPort.write((uint32_t) (&initBlock) & 0xFFFF);
  
  registerAddressPort.write(2);
  registerDataPort.write(((uint32_t) (&initBlock) >> 16) & 0xFFFF);
}

amd_am79c973::~amd_am79c973() {}

void amd_am79c973::activate() {
  registerAddressPort.write(0);
  registerDataPort.write(0x41);
  
  registerAddressPort.write(4);
  uint32_t tmp = registerDataPort.read() | 0xC00;
  registerAddressPort.write(4);
  registerDataPort.write(tmp);
  
  registerAddressPort.write(0);
  registerDataPort.write(0x42);
}

int amd_am79c973::reset() {
  resetPort.read();
  resetPort.write(0);
  return 10;
}

common::uint32_t amd_am79c973::handleInterrupt(common::uint32_t esp) {
  printf("INTERRUPT FROM NETWORK CARD\n");
  
  registerAddressPort.write(0);
  uint32_t tmp = registerDataPort.read();
  
  if((tmp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n");
  if((tmp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
  if((tmp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n");
  if((tmp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");
  if((tmp & 0x0400) == 0x0400) printf("AMD am79c973 DATA RECEVED\n");
  if((tmp & 0x0200) == 0x0200) printf("AMD am79c973 DATA SENT\n");
  
  registerAddressPort.write(0);
  registerDataPort.write(tmp);

  if((tmp & 0x0100) == 0x0100) printf("AMD DONE\n");
  
  return esp;
}
