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
  }
}

void AdvancedTechnologyAttachment::read28(common::uint32_t sector) {
  
}

void AdvancedTechnologyAttachment::write28(common::uint32_t sector, common::uint8_t* data, int count) {
  
}

void AdvancedTechnologyAttachment::flush() {
  
}
