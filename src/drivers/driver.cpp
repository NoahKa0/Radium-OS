#include <drivers/driver.h>
#include <drivers/net/amd_am79c973.h>
#include <drivers/net/broadcom_BCM5751.h>
#include <drivers/storage/ata.h>
#include <hardware/pci.h>
#include <hardware/interrupts.h>

using namespace sys::drivers;
using namespace sys::hardware;

Driver::Driver() {
    
}

Driver::~Driver() {
    
}

void Driver::activate() {
    
}

int Driver::reset() {
    return 0;
}

void Driver::deactivate() {
    
}

Driver* Driver::getDriver(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager) {
  Driver* ret = 0;

  // Storage devices
  ret = storage::AdvancedTechnologyAttachment::getDriver(device, interruptManager);
  if(ret != 0) return ret;

  // Network cards
  ret = amd_am79c973::getDriver(device, interruptManager);
  if(ret != 0) return ret;

  ret = broadcom_BCM5751::getDriver(device, interruptManager);
  if(ret != 0) return ret;

  return ret;
}


DriverManager::DriverManager() {
    numDrivers = 0;
}

void DriverManager::addDriver(Driver* driver) {
    drivers[numDrivers] = driver;
    numDrivers++;
}

void DriverManager::activateAll() {
    for(int i = 0; i < numDrivers; i++) {
        drivers[i]->activate();
    }
}
