#include <drivers/audio/AC97.h>
#include <audio/Audio.h>

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
:AudioDriver(),
port8Bit(0x00),
port16Bit(0x00),
port32Bit(0x00),
InterruptHandler(device->interrupt + 0x20, interruptManager){
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
    this->descriptorsOut[i].length = (DescEntryLength * 2) - 1;
    this->descriptorsOut[i].options = IoC;
    if(descArraySize-1 == i) {
      this->descriptorsOut[i].options |= LastDescEntry;
    }
    for(uint32_t x = 0; x < DescEntryLength * Channels; x++) {
      this->descriptorsOut[i].address[x] = 0;
    }
  }

  this->writeEntry = 0;
  this->writePosition = 0;
  this->readEntry = 0;
  this->volume = 0;
  this->active = false;
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
  this->csr32w(Out | CurrentEntry, 0);
  this->csr32w(Out | LastValidEntry, descArraySize);

  this->start();

  printf("\n\n\n\nDriver registered!\n\n\n");
  sys::audio::Audio::activeAudioManager->registerDriver(this);
}

void AC97::start() {
  if(!this->active && this->readEntry != this->writeEntry && this->volume != 0) {
    printf("\n\nSending start signal!\n\n");
    uint8_t tmp = BufferTransfer | BufferInterruptOnComplete | BufferInterruptOnLast | BufferInterruptOnError;
    this->csr8w(Out | BufferControl, tmp);
    this->active = true;
  }
}

void AC97::stop() {
  printf("\n\nSending stop signal!\n\n");
  uint8_t tmp = BufferInterruptOnComplete | BufferInterruptOnLast | BufferInterruptOnError;
  this->csr8w(Out | BufferControl, tmp);
  this->active = false;
  this->writeEntry = this->readEntry;
  this->writePosition = 0;
}

bool AC97::isBigEndian() {
  return false; // We'll see
}

uint8_t AC97::getChannels() {
  return Channels;
}

uint8_t AC97::getSampleSize() {
  return 2;
}

void AC97::write(common::uint8_t* samples, common::uint32_t sizeInBytes) {
  uint32_t bytesReady = this->samplesReady() * Channels * 2;
  uint32_t written = 0;
  bool play = false;
  if(sizeInBytes > bytesReady) {
    printf("SOUND OVERFLOW!!\n");
    sizeInBytes = bytesReady;
  }
  while(written < sizeInBytes) {
    uint32_t readyInEntry = (DescEntryLength * Channels * 2) - this->writePosition;
    uint32_t write = (sizeInBytes - written);
    if(write > readyInEntry) {
      write = readyInEntry;
    }
    uint8_t* target = (uint8_t*) this->descriptorsOut[this->writeEntry].address;
    for(uint32_t i = 0; i < write; i++) {
      target[i] = samples[written + i];
    }
    written += write;
    this->writePosition += write;
    if(this->writePosition >= (DescEntryLength * Channels * 2)) {
      this->writeEntry = (this->writeEntry + 1) % descArraySize;
      this->writePosition = 0;
      play = true;
    }
  }
  if(play) {
    this->start();
  }
}

uint32_t AC97::samplesReady() {
  if(this->readEntry == this->writeEntry) {
    return DescEntryLength * descArraySize / 2;
  }
  int32_t diff = 0;
  if(this->writeEntry < this->readEntry) {
    diff = this->readEntry - this->writeEntry;
  } else {
    diff = descArraySize - this->writeEntry + this->readEntry - 1;
  }
  return DescEntryLength * (diff - 1);
}

void AC97::setVolume(uint8_t volume) {
  this->volume = volume;
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

  this->readEntry = current;
  if(this->readEntry == this->writeEntry && this->active) {
    this->stop();
  }

  uint16_t tmp = this->csr16r(Out | StatusRegister);
  if(tmp & 1 != 0) {
    printf("Read: ");
    printHex32(this->readEntry);
    printf(" write: ");
    printHex32(this->writeEntry);
    printf("\n");
    this->active = false;
  }

  if(tmp & 2 != 0) {
    printf("End of transfer!\n");
  }

  if(tmp & 2 != 0) {
    printf("Last buffer interrupt\n");
  }
  if(tmp & 4 != 0) {
    printf("IOC interrupt\n");
  }
  if(tmp & 8 != 0) {
    printf("Fifo error interrupt\n");
    this->start();
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
