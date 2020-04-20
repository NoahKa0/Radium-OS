#include <filesystem/partition/partitionManager.h>
#include <filesystem/partition/mbr.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem::partition;
using namespace sys::drivers::storage;

PartitionManager* PartitionManager::activePartitionManager = 0;

PartitionManager::PartitionManager() {
  if(PartitionManager::activePartitionManager != 0) {
    delete PartitionManager::activePartitionManager;
  }
  PartitionManager::activePartitionManager = this;

  for(int i = 0; i < 32; i++) {
    this->devices[i] = 0;
  }
  this->deviceCount = 0;
}

PartitionManager::~PartitionManager() {

}

void PartitionManager::registerDevice(StorageDevice* device) {
  if(this->deviceCount >= 32) {
    return;
  }

  deviceEntry* entry = (deviceEntry*) MemoryManager::activeMemoryManager->malloc(sizeof(deviceEntry));
  entry->partition = 0;
  entry->device = device;

  this->devices[this->deviceCount] = entry;
  this-deviceCount++;
}

uint32_t PartitionManager::numDevices() {
  return this->deviceCount;
}

StorageDevice* PartitionManager::getDevice(uint32_t device) {
  if(device >= this->deviceCount) {
    return 0;
  }
  return this->devices[device]->device;
}

partitionEntry* PartitionManager::findPartitions(StorageDevice* device) {
  partitionEntry* partitions = 0;

  if(MBR::isMBR(device)) {
    partitions = MBR::findPartitions(device);
  }

  return partitions;
}

bool PartitionManager::mount(uint32_t device) {
  if(device >= this->deviceCount || this->devices[device]->partition != 0) {
    return false;
  }
  partitionEntry* partition = this->findPartitions(this->devices[device]->device);
  this->devices[device]->partition = partition;

  while(partition != 0) {
    partition->partition->findFileSystem();
    partition = partition->next;
  }
  return true;
}

bool PartitionManager::unmount(uint32_t device) {
  if(device >= this->deviceCount || this->devices[device]->partition == 0) {
    return false;
  }
  partitionEntry* partition = this->devices[device]->partition;
  this->devices[device]->partition = 0;
  partitionEntry* next = 0;
  while(partition != 0) {
    next = partition->next;
    delete partition->partition;
    delete partition;
    partition = next;
  }
  return true;
}

uint32_t PartitionManager::numPartitions(uint32_t device) {
  if(device >= this->deviceCount || this->devices[device]->partition == 0) {
    return false;
  }
  partitionEntry* partition = this->devices[device]->partition;

  uint32_t numPartitions = 0;
  while(partition != 0) {
    numPartitions++;
    partition = partition->next;
  }
  return numPartitions;
}

Partition* PartitionManager::getPartition(uint32_t device, uint32_t partition) {
  if(device >= this->deviceCount || this->devices[device]->partition == 0) {
    return 0;
  }
  partitionEntry* entry = this->devices[device]->partition;

  uint32_t numPartitions = 0;
  while(entry != 0 && numPartitions != partition) {
    numPartitions++;
    entry = entry->next;
  }
  if(numPartitions == partition && entry != 0) {
    return entry->partition;
  }
  return 0;
}

common::uint8_t* PartitionManager::getFullFile(String* filepath) {
  return 0;
}
