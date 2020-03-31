#include <filesystem/partition/mbr.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem::partition;
using namespace sys::drivers::storage;

void printf(char*);
void printHex8(uint8_t);

void MBR::readMBR(StorageDevice* ata) {
  MBRStructure mbr;
  ata->read(0, (uint8_t*) &mbr, sizeof(MBRStructure));

  if(mbr.magicnumber != 0xAA55) {
    printf("Illegal mbr\n");

    return;
  }

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
    fat::readBPB(ata, mbr.primaryPartition[i].startLba);
  }
}
