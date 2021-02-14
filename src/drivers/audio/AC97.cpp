#include <drivers/audio/AC97.h>

using namespace sys;
using namespace sys::common;
using namespace sys::hardware;
using namespace sys::drivers;
using namespace sys::drivers::audio;


void printHex8(uint8_t);
void printHex32(uint32_t);
void printHex64(uint64_t);

void printf(const char*);


void AC97::csr8w(uint32_t reg, uint8_t value) {
  if(this->device->memoryMapped) {
    *((uint8_t*) (this->baseAddr + reg)) = value;
  } else {
    this->port8Bit.setPortNumber(this->basePort + reg);
    this->port8Bit.write(value);
  }
}
void AC97::csr16w(uint32_t reg, uint16_t value) {
  if(this->device->memoryMapped) {
    *((uint16_t*) (this->baseAddr + reg)) = value;
  } else {
    this->port16Bit.setPortNumber(this->basePort + reg);
    this->port16Bit.write(value);
  }
}
void AC97::csr32w(uint32_t reg, uint32_t value) {
  if(this->device->memoryMapped) {
    *((uint32_t*) (this->baseAddr + reg)) = value;
  } else {
    this->port32Bit.setPortNumber(this->basePort + reg);
    this->port32Bit.write(value);
  }
}

uint8_t AC97::csr8r(uint32_t reg) {
  if(this->device->memoryMapped) {
    return *((uint8_t*) (this->baseAddr + reg));
  } else {
    this->port8Bit.setPortNumber(this->basePort + reg);
    return this->port8Bit.read();
  }
}
uint16_t AC97::csr16r(uint32_t reg) {
  if(this->device->memoryMapped) {
    return *((uint16_t*) (this->baseAddr + reg));
  } else {
    this->port16Bit.setPortNumber(this->basePort + reg);
    return this->port16Bit.read();
  }
}
uint32_t AC97::csr32r(uint32_t reg) {
  if(this->device->memoryMapped) {
    return *((uint32_t*) (this->baseAddr + reg));
  } else {
    this->port32Bit.setPortNumber(this->basePort + reg);
    return this->port32Bit.read();
  }
}


AC97::AC97(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager)
:Driver(),
port8Bit(0x00),
port16Bit(0x00),
port32Bit(0x00),
InterruptHandler(device->interrupt + 0x20, interruptManager){
  if(device->memoryMapped != true) {
    printf("AC97 bar is not set to memory mapped!");
  }
  this->device = device;

  this->baseAddr = (uint8_t*) device->bar[1].address;
  this->basePort = (uint32_t) device->bar[1].address;

  this->mixAddr = (uint8_t*) device->bar[0].address;
  this->mixPort = (uint32_t) device->bar[0].address;

  // Enable MAC memory space decode and bus mastering.
  uint32_t pciCommandRead = this->device->read(0x04);
  pciCommandRead |= 0x07;
  this->device->write(0x04, pciCommandRead);

  this->descriptorsOut = (AC97Desc*) MemoryManager::activeMemoryManager->mallocalign(sizeof(AC97Desc) * descArraySize, 8);
  for(uint32_t i = 0; i < descArraySize; i++) {
    this->descriptorsOut[i].address = (uint16_t*) MemoryManager::activeMemoryManager->mallocalign(sizeof(uint16_t) * DescEntryLength * Channels, 8);
    this->descriptorsOut[i].length = DescEntryLength - 1;
    this->descriptorsOut[i].options = IoC;
    if(descArraySize-1 == i) {
      this->descriptorsOut[i].options |= LastDescEntry;
    }
    for(uint32_t x = 0; x < DescEntryLength * Channels; x++) {
      /**
       * Debug, 48000 Herz devided by 64 gives us 720 points or 360 herz, should be hearable.
       */
      uint32_t highOrLow = x / Channels / 64;
      this->descriptorsOut[i].address[x] = highOrLow % 2 == 1 ? 0 : 0x4F4F;
    }
    printf("-");
  }

  printf("!");

  this->writeEntry = 0;
  this->writePosition = 0;
}

AC97::~AC97() {
  for(uint32_t i = 0; i < descArraySize; i++) {
    MemoryManager::activeMemoryManager->free(this->descriptorsOut[i].address);
  }
  MemoryManager::activeMemoryManager->free(this->descriptorsOut);
}

void AC97::activate() {
  // TODO
  uint32_t tmp = this->csr32r(GlobalControl);
  tmp &= ~(ReservedControlBits | LinkShutoff); // Unset link shutoff and reserved bits.
  tmp &= ~(Sample20BitEnable); // Disable 20 bit sampling.

  if((tmp & ColdReset) == 0) {
    tmp |= ColdReset;
  } else {
    tmp |= WarmReset;
  }

  this->csr32w(GlobalControl, tmp);

  this->setVolume(255);

  this->csr8w(Out | BufferControl, BufferReset);
  while(this->csr8r(Out | BufferControl) & BufferReset != 0) {
    // Waiting
    printf(".");
  }

  this->csr32w(Out | DescArrayAddr, (uint32_t) this->descriptorsOut);
  //this->csr32w(Out | CurrentEntry, 0);
  this->csr32w(Out | LastValidEntry, descArraySize);

  tmp = BufferTransfer | BufferInterruptOnComplete | BufferInterruptOnLast | BufferInterruptOnError;
  this->csr8w(Out | BufferControl, tmp);
}

void AC97::setVolume(uint8_t volume) {
  volume = ~volume; // IDK why but volume is inversed?
  volume = (volume >> 3) & 0x1F;
  uint16_t masterVolume = volume & (volume << 6);

  volume = volume & 0x0F;
  uint16_t pcmVolume = volume & (volume << 8);
  
  this->port16Bit.setPortNumber(this->mixPort + 0x02); // 0x02 Master volume
  this->port16Bit.write(masterVolume);

  this->port16Bit.setPortNumber(this->mixPort + 0x18); // 0x18 PCM output volume
  this->port16Bit.write(pcmVolume);
}

int AC97::reset() {
  return 0;
}

void AC97::deactivate() {}

uint32_t AC97::handleInterrupt(uint32_t esp) {
  uint16_t current = (uint16_t) this->csr32r(Out | CurrentEntry);
  printf("\nAC97 INT: ");
  printHex32(current);
  printf("\n");

  uint16_t tmp = this->csr16r(Out | StatusRegister);
  if(tmp & 1 != 0) {
    printf("DMA halted!\n");
  } else {
    printf("DMA transfering!\n");
  }

  if(tmp & 2 != 0) {
    printf("End of transfer!\n");
  } else {
    printf("Transfering!\n");
  }

  if(tmp & 4 != 0) {
    printf("IOC interrupt\n");
  }
  if(tmp & 8 != 0) {
    printf("Fifo error interrupt\n");
  }

  if(tmp & 2 != 0) {
    printf("Last buffer interrupt\n");
    this->csr32w(Out | CurrentEntry, 0);
    tmp = BufferTransfer | BufferInterruptOnComplete | BufferInterruptOnLast | BufferInterruptOnError;
    this->csr8w(Out | BufferControl, tmp);
  }

  this->csr16w(Out | StatusRegister, 0x1C);
  
  return esp;
}

Driver* AC97::getDriver(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager) {
  Driver* ret = 0;

  if(device->classId == 0x04 && device->subclassId == 0x01) {
    ret = new AC97(device, interruptManager);
  }

  return ret;
}
