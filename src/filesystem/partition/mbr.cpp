#include <filesystem/partition/mbr.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem::partition;
using namespace sys::drivers::storage;

void printf(char*);
void printHex8(uint8_t);

bool MBR::isMBR(StorageDevice* device) {
  MBRStructure mbr;
  device->read(0, (uint8_t*) &mbr, sizeof(MBRStructure));

  return mbr.magicnumber == 0xAA55;
}

partitionEntry* MBR::findPartitions(StorageDevice* device) {
  MBRStructure mbr;
  device->read(0, (uint8_t*) &mbr, sizeof(MBRStructure));

  if(mbr.magicnumber != 0xAA55) {
    printf("Illegal mbr\n");

    return 0;
  }

  partitionEntry* ret = 0;
  partitionEntry* last = 0;

  for(uint8_t i = 0; i < 4; i++) {
    if(mbr.primaryPartition[i].partitionId == 0) {
      continue;
    }
    mbr.primaryPartition[i];
    printf("Partition ");
    printHex8(i);
    printf(", type: ");
    printHex8(mbr.primaryPartition[i].partitionId);
    if(mbr.primaryPartition[i].bootable & 0x80 == 0x80) {
      printf(" bootable\n");
    } else {
      printf(" not bootable\n");
    }
    partitionEntry* partition = (partitionEntry*) MemoryManager::activeMemoryManager->malloc(sizeof(partitionEntry));
    partition->partition = new Partition(device, mbr.primaryPartition[i].startLba, mbr.primaryPartition[i].length);
    partition->next = 0;

    if(last != 0) {
      last->next = partition;
    }
    last = partition;
    if(ret == 0) {
      ret = last;
    }
  }
  return ret;
}
