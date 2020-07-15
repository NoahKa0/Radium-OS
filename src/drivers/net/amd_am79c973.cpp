#include <drivers/net/amd_am79c973.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);
void setSelectedEthernetDriver(EthernetDriver* drv);

amd_am79c973::amd_am79c973(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager)
: EthernetDriver(),
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
  
  // This descriptor is no longer needed.
  delete device;
  
  setSelectedEthernetDriver(this); // Make this instance accessable in kernel.cpp
}

amd_am79c973::~amd_am79c973() {}

void amd_am79c973::activate() {
  asm("cli");
  registerAddressPort.write(0);
  registerDataPort.write(0x41);
  
  registerAddressPort.write(4);
  uint32_t tmp = registerDataPort.read();
  registerAddressPort.write(4);
  registerDataPort.write(tmp | 0xC00);
  
  registerAddressPort.write(0);
  registerDataPort.write(0x42);
  
  asm("sti");
}

int amd_am79c973::reset() {
  resetPort.read();
  resetPort.write(0);
  return 10;
}

common::uint32_t amd_am79c973::handleInterrupt(common::uint32_t esp) {
  registerAddressPort.write(0);
  uint32_t tmp = registerDataPort.read();
  
  if((tmp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n");
  if((tmp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
  if((tmp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n");
  if((tmp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");
  if((tmp & 0x0400) == 0x0400) receive();
  //if((tmp & 0x0200) == 0x0200) printf("AMD am79c973 DATA SENT\n");
  
  registerAddressPort.write(0);
  registerDataPort.write(tmp);

  //if((tmp & 0x0100) == 0x0100) printf("AMD DONE\n");
  return esp;
}

void amd_am79c973::send(common::uint8_t* buffer, int size) {
  int sendDescriptor = currentSendBuffer;
  currentSendBuffer = (currentSendBuffer+1) % 8;
  
  if(size > 1518) {
    size = 1518;
  }
  
  uint8_t* src = buffer + size -1;
  uint8_t* dest = (uint8_t*) (sendBufferDescr[sendDescriptor].address + size -1);
  
  while(src >= buffer) {
    *dest = *src;
    dest--;
    src--;
  }
  
  sendBufferDescr[sendDescriptor].available = 0;
  sendBufferDescr[sendDescriptor].flags2 = 0;
  sendBufferDescr[sendDescriptor].flags = 0x8300F000 | ((uint16_t) ((-size) & 0xFFF));
  
  registerAddressPort.write(0);
  registerDataPort.write(0x48);
}

void amd_am79c973::receive() {
  while((reciveBufferDescr[currentReciveBuffer].flags & 0x80000000) == 0) {
    if(!(reciveBufferDescr[currentReciveBuffer].flags & 0x40000000) // Check for error bits
    && (reciveBufferDescr[currentReciveBuffer].flags & 0x03000000) == 0x03000000) // Check for startOfPacket and EndOfPacket bits
    {
      uint32_t size = reciveBufferDescr[currentReciveBuffer].flags & 0xFFF;
      if(size > 64) { // Remove checksum
        size -= 4;
      }
      uint8_t* buffer = (uint8_t*) (reciveBufferDescr[currentReciveBuffer].address);
      if(this->etherFrameProvider != 0) {
        if(this->etherFrameProvider->onRawDataRecived(buffer, size)) {
          send(buffer, size);
        }
      }
    }
    
    reciveBufferDescr[currentReciveBuffer].flags2 = 0;
    reciveBufferDescr[currentReciveBuffer].flags = 0x8000F7FF;
    
    currentReciveBuffer = (currentReciveBuffer+1) % 8;
  }
}

uint64_t amd_am79c973::getMacAddress() {
  return this->initBlock.physicalAddress;
}

uint32_t amd_am79c973::getIpAddress() {
  return initBlock.logicalAddress;
}

void amd_am79c973::setIpAddress(uint32_t ip) {
  initBlock.logicalAddress = ip;
}

Driver* amd_am79c973::getDriver(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager) {
  Driver* ret = 0;
  if(device->vendorId == 0x1022 // AMD
  && (device->deviceId == 0x2000 || device->deviceId == 0x2001))
  {
    ret = new amd_am79c973(device, interruptManager);
  }
  return ret;
}
