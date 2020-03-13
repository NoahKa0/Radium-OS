#include <drivers/ata.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);


AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(common::uint16_t portBase, bool master)
: dataPort(portBase),
errorPort(portBase + 1),
sectorCountPort(portBase + 2),
lbaLowPort(portBase + 3),
lbaMidPort(portBase + 4),
lbaHiPort(portBase + 5),
devicePort(portBase + 6),
commandPort(portBase + 7),
controlPort(portBase + 0x206)
{
  this->bytesPerSector = 512;
  this->master = master;
}

AdvancedTechnologyAttachment::~AdvancedTechnologyAttachment() {}

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
  
  for(uint16_t i = 0; i < 256; i++) {
    uint16_t data = dataPort.read();
    printHex8(data & 0x00FF);
    printf(", ");
  }
}

void AdvancedTechnologyAttachment::read28(uint32_t sector, uint8_t* data, int count) {
  if(sector & 0xF0000000) {
    return;
  }
  if(count > bytesPerSector) {
    return;
  }
  
  devicePort.write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  errorPort.write(0);
  sectorCountPort.write(1);
  
  lbaLowPort.write(sector & 0x000000FF);
  lbaMidPort.write((sector & 0x0000FF00) >> 8);
  lbaHiPort.write((sector & 0x00FF0000) >> 16);
  commandPort.write(0x20);
  
  uint8_t status = commandPort.read();
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
    status = commandPort.read();
  }
  if(status & 0x01) {
    printf("ERROR!");
    return;
  }
  
  printf("Reading from disk: ");
  
  for(uint16_t i = 0; i < count; i+=2) {
    uint16_t rdata = dataPort.read();
    
    data[i] = rdata & 0x00FF;
    if(i+1 < count) {
      data[i+1] = (rdata >> 8) & 0x00FF;
    }
    
    printHex8(rdata & 0x00FF);
    printf(", ");
  }
  for(uint16_t i = count+(count%2); i < bytesPerSector; i+=2) {
    dataPort.read();
  }
  printf("\n");
}

void AdvancedTechnologyAttachment::write28(uint32_t sector, uint8_t* data, int count) {
  if(sector & 0xF0000000) {
    return;
  }
  if(count > bytesPerSector) {
    return;
  }
  
  devicePort.write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  errorPort.write(0);
  sectorCountPort.write(1);
  
  lbaLowPort.write(sector & 0x000000FF);
  lbaMidPort.write((sector & 0x0000FF00) >> 8);
  lbaHiPort.write((sector & 0x00FF0000) >> 16);
  commandPort.write(0x30);
  
  printf("Writing to disk: ");
  
  for(uint16_t i = 0; i < count; i+=2) {
    uint16_t wdata = data[i];
    if(i+1 < count) {
      wdata = wdata | (((uint16_t) data[i+1]) << 8);
    }
    
    dataPort.write(wdata);
    
    printHex8(wdata & 0x00FF);
    printf(", ");
  }
  for(uint16_t i = count+(count%2); i < bytesPerSector; i+=2) {
    dataPort.write(0x0000);
  }
  printf("\n");
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
