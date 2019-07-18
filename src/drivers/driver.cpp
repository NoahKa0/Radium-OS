#include <drivers/driver.h>

using namespace sys::drivers;

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
