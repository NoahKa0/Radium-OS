#include <filesystem/partition/partition.h>
#include <filesystem/fat.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem::partition;
using namespace sys::drivers::storage;

Partition::Partition(StorageDevice* device, common::uint64_t startLBA, common::uint64_t length) {
  this->device = device;
  this->startLBA = startLBA;
  this->length = length;
  this->endLBA = startLBA + length;
}

Partition::~Partition() {

}

void Partition::read(common::uint64_t sector, common::uint8_t* data, int length) {
  this->device->read(sector + this->startLBA, data, length);
}

void Partition::write(common::uint64_t sector, common::uint8_t* data, int length) {
  this->device->write(sector + this->startLBA, data, length);
}

uint64_t Partition::getSectorCount() {
  return this->length;
}

StorageDevice* Partition::getDevice() {
  return this->device;
}

void Partition::findFileSystem() {
  Fat::readBPB(this);
}
