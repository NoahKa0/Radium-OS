#include <drivers/storage/ata.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::drivers::storage;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);


AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager)
: dataPort(0),
errorPort(0),
sectorCountPort(0),
lbaLowPort(0),
lbaMidPort(0),
lbaHiPort(0),
devicePort(0),
commandPort(0),
controlPort(0)
{
  uint32_t primaryMasterPortBase = (uint32_t) device->bar[0].address;
  if(primaryMasterPortBase == 0 || primaryMasterPortBase == 1) {
    primaryMasterPortBase = 0x1F0;
  }

  dataPort.setPortNumber(primaryMasterPortBase);
  errorPort.setPortNumber(primaryMasterPortBase + 1);
  sectorCountPort.setPortNumber(primaryMasterPortBase + 2);
  lbaLowPort.setPortNumber(primaryMasterPortBase + 3);
  lbaMidPort.setPortNumber(primaryMasterPortBase + 4);
  lbaHiPort.setPortNumber(primaryMasterPortBase + 5);
  devicePort.setPortNumber(primaryMasterPortBase + 6);
  commandPort.setPortNumber(primaryMasterPortBase + 7);
  controlPort.setPortNumber(primaryMasterPortBase + 0x206);

  this->bytesPerSector = 512;
  this->master = true;
}

AdvancedTechnologyAttachment::~AdvancedTechnologyAttachment() {}

void AdvancedTechnologyAttachment::activate() {
  this->identify();
  this->registerDevice();
}

void AdvancedTechnologyAttachment::identify() {
  devicePort.write(master ? 0xA0 : 0xB0);
  controlPort.write(0);
  
  devicePort.write(0xA0);
  uint8_t status = commandPort.read();
  if(status == 0xFF) {
    printf("NO DEVICE!");
    return;
  }
  
  devicePort.write(master ? 0xA0 : 0xB0);
  sectorCountPort.write(0);
  lbaLowPort.write(0);
  lbaMidPort.write(0);
  lbaHiPort.write(0);
  commandPort.write(0xEC);
  
  status = commandPort.read();
  if(status == 0x00) {
    printf("NO DEVICE!");
    return;
  }
  
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
    status = commandPort.read();
  }
  if(status & 0x01) {
    printf("ERROR!");
    return;
  }
  
  uint16_t data[256];

  for(uint16_t i = 0; i < 256; i++) {
    data[i] = dataPort.read();
  }

  uint64_t size28 = data[61] << 16 | data[60];
  uint64_t size48 = data[101] << 16 | data[100] | data[103] << 48 | data[102] << 32;
  if(size48 == 0) {
    this->sectorCount = size28;
  } else {
    this->sectorCount = size48;
  }
}

void AdvancedTechnologyAttachment::read(uint64_t sector, uint8_t* data, int count) {
  if(sector & 0xF00000000000000) {
    return;
  }
  if(count > bytesPerSector) {
    return;
  }
  
  devicePort.write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  errorPort.write(0);

  sectorCountPort.write(0);
  lbaLowPort.write((sector >> 24) & 0x00000000000000FF);
  lbaMidPort.write((sector >> 32) & 0x00000000000000FF);
  lbaHiPort.write((sector >> 40) & 0x00000000000000FF);

  sectorCountPort.write(1);
  lbaLowPort.write(sector & 0x00000000000000FF);
  lbaMidPort.write((sector >> 8) & 0x00000000000000FF);
  lbaHiPort.write((sector >> 16) & 0x00000000000000FF);
  commandPort.write(0x24);
  
  uint8_t status = commandPort.read();
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
    status = commandPort.read();
  }
  if(status & 0x01) {
    printf("ERROR!");
    return;
  }
  
  for(uint16_t i = 0; i < count; i+=2) {
    uint16_t rdata = dataPort.read();
    
    data[i] = rdata & 0x00FF;
    if(i+1 < count) {
      data[i+1] = (rdata >> 8) & 0x00FF;
    }
  }
  for(uint16_t i = count+(count%2); i < bytesPerSector; i+=2) {
    dataPort.read();
  }
}

void AdvancedTechnologyAttachment::write(uint64_t sector, uint8_t* data, int count) {
  if(sector & 0xF0000000) {
    return;
  }
  if(count > bytesPerSector) {
    return;
  }
  
  devicePort.write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  errorPort.write(0);

  sectorCountPort.write(0);
  lbaLowPort.write((sector >> 24) & 0x00000000000000FF);
  lbaMidPort.write((sector >> 32) & 0x00000000000000FF);
  lbaHiPort.write((sector >> 40) & 0x00000000000000FF);

  sectorCountPort.write(1);
  lbaLowPort.write(sector & 0x00000000000000FF);
  lbaMidPort.write((sector >> 8) & 0x00000000000000FF);
  lbaHiPort.write((sector >> 16) & 0x00000000000000FF);
  commandPort.write(0x34);
  
  for(uint16_t i = 0; i < count; i+=2) {
    uint16_t wdata = data[i];
    if(i+1 < count) {
      wdata = wdata | (((uint16_t) data[i+1]) << 8);
    }
    
    dataPort.write(wdata);
  }
  for(uint16_t i = count+(count%2); i < bytesPerSector; i+=2) {
    dataPort.write(0x0000);
  }
}

void AdvancedTechnologyAttachment::flush() {
  devicePort.write(master ? 0xE0 : 0xF0);
  commandPort.write(0xE7);
  
  uint8_t status = commandPort.read();
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
    status = commandPort.read();
  }
  if(status & 0x01) {
    printf("ERROR!");
    return;
  }
}

uint64_t AdvancedTechnologyAttachment::getSectorCount() {
  return this->sectorCount;
}
