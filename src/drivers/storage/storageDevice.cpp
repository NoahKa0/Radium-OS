#include <drivers/storage/storageDevice.h>
#include <filesystem/partition/mbr.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::drivers::storage;
using namespace sys::hardware;
using namespace sys::filesystem::partition;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);


StorageDevice::StorageDevice():Driver() {}

StorageDevice::~StorageDevice() {}

void StorageDevice::identify() {}

void StorageDevice::read(uint64_t sector, uint8_t* data, int count) {}

void StorageDevice::write(uint64_t sector, uint8_t* data, int count) {}

void StorageDevice::flush() {}

common::uint64_t StorageDevice::getSectorCount() {
  return 0;
}

void StorageDevice::findPartitions() {
  MBR::readMBR(this);
}
