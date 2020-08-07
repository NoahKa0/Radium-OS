#include <hardware/pci.h>
#include <drivers/net/amd_am79c973.h>
#include <drivers/net/broadcom_BCM5751.h>
#include <drivers/storage/ata.h>

using namespace sys::hardware;
using namespace sys::common;
using namespace sys::drivers;

void printf(const char* str);
void printHex32(uint32_t num);

PeripheralComponentDeviceDescriptor::PeripheralComponentDeviceDescriptor(PeripheralComponentInterconnect* pci) {
  this->pci = pci;
}
PeripheralComponentDeviceDescriptor::~PeripheralComponentDeviceDescriptor() {}

uint32_t PeripheralComponentDeviceDescriptor::read(uint32_t registerOffset) {
  return this->pci->read(this->bus, this->device, this->function, registerOffset);
}

void PeripheralComponentDeviceDescriptor::write(uint32_t registerOffset, uint32_t value) {
  this->pci->write(this->bus, this->device, this->function, registerOffset, value);
}



PeripheralComponentInterconnect::PeripheralComponentInterconnect()
: dataPort(0xCFC),
commandPort(0xCF8)
{
  
}

PeripheralComponentInterconnect::~PeripheralComponentInterconnect() {}


uint32_t PeripheralComponentInterconnect::read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset)
{
  uint32_t id =
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);
  
  commandPort.write(id);
  uint32_t result = dataPort.read();
  return result >> (8* (registeroffset % 4));
}

void PeripheralComponentInterconnect::write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value)
{
  uint32_t id =
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);
  commandPort.write(id);
  dataPort.write(value); 
}

bool PeripheralComponentInterconnect::hasFunctions(uint16_t bus, uint16_t device) {
  return read(bus, device, 0, 0x0E) & (1<<7);
}

void PeripheralComponentInterconnect::selectDrivers(DriverManager* driverManager, InterruptManager* interruptManager) {
  for(int bus = 0; bus < 8; bus++) {
    for(int device = 0; device < 32; device++) {
      int maxFunction = hasFunctions(bus, device) ? 8 : 1;
      for(int function = 0; function < maxFunction; function++) {
        PeripheralComponentDeviceDescriptor* dd = getDeviceDescriptor(bus, device, function);
        if(dd->vendorId == 0x0000 || dd->vendorId == 0xFFFF) {
          continue;
        }
        
        for(int barNum = 0; barNum < 6; barNum++) {
          BaseAddressRegister bar = getBaseAddressRegister(bus, device, function, barNum);

          // NOTICE I should remove portBase, addressBase and memoryMapped from the device descriptors and use the base address descriptors in the drivers, i will do this later.
          if(bar.address && (bar.type == inputOutput)) {
            dd->portBase = (uint32_t) bar.address;
          }
          if(bar.address && (bar.type == memoryMapping)) {
            dd->addressBase = (uint32_t) bar.address;
            dd->memoryMapped = true;
          }

          dd->bar[barNum].address = bar.address;
          dd->bar[barNum].prefetchable = bar.prefetchable;
          dd->bar[barNum].size = bar.size;
          dd->bar[barNum].type = bar.type;
        }
        
        Driver* driver = getDriver(dd, interruptManager);
        if(driver != 0) {
          driverManager->addDriver(driver);
        }
        
        printf("PCI BUS ");
        printHex32(bus);
        printf(", device ");
        printHex32(device);
        printf(", function ");
        printHex32(function);
        
        printf(" = vendor ");
        printHex32(dd->vendorId);
        
        printf(", device ");
        printHex32(dd->deviceId);
        
        printf("\n");
      }
    }
  }
}

PeripheralComponentDeviceDescriptor* PeripheralComponentInterconnect::getDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t function) {
  PeripheralComponentDeviceDescriptor* result = new PeripheralComponentDeviceDescriptor(this);

  for(int i = 0; i < 6; i++) {
    result->bar[i].address = 0;
    result->bar[i].type = memoryMapping;
    result->bar[i].size = 0;
    result->bar[i].prefetchable = 0;
  }

  result->bus = bus;
  result->device = device;
  result->function = function;
  
  result->vendorId = read(bus, device, function, 0x00);
  result->deviceId = read(bus, device, function, 0x02);
  
  result->classId = read(bus, device, function, 0x0B);
  result->subclassId = read(bus, device, function, 0x0A);
  result->interfaceId = read(bus, device, function, 0x09);
  
  result->revision = read(bus, device, function, 0x08);
  result->interrupt = read(bus, device, function, 0x3C) & 0xF;

  // Default values
  result->addressBase = 0;
  result->memoryMapped = false;
  result->portBase = 0;

  return result;
}

BaseAddressRegister PeripheralComponentInterconnect::getBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar) {
  BaseAddressRegister result;
  
  uint32_t headerType = read(bus, device, function, 0x0E) & 0x7F;
  int maxBars = 6-(4*headerType);
  
  if(bar >= maxBars) {
    return result;
  }
  
  uint32_t barValue = read(bus, device, function, 0x10 + 4*bar);
  
  result.type = (barValue & 0x1) ? inputOutput : memoryMapping;
  
  if(result.type == memoryMapping) {
    // This is used to see where memory can be mapped, and of what type.
    switch((barValue >> 1) & 0x3) {
      case 0:
      case 1:
      case 2:
        break;
    }
    
    result.prefetchable = ((barValue >> 3) & 0x1) == 0x1;
    result.address = (uint8_t*) (barValue & ~0xF);
  } else {
    result.address = (uint8_t*) (barValue & ~0x3);
    result.prefetchable = false;
  }
  
  return result;
}

void setNicName(char* name);

Driver* PeripheralComponentInterconnect::getDriver(PeripheralComponentDeviceDescriptor* dev, InterruptManager* interruptManager) {
  
  Driver* driver = Driver::getDriver(dev, interruptManager);
  
  if(driver == 0) {
    delete dev;
  }
  
  return driver;
}

