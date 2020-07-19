#include <cli/sd.h>
#include <drivers/storage/storageDevice.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::filesystem::partition;

void printf(const char*);
void printHex32(uint32_t);
void printHex8(uint8_t);


CmdSD::CmdSD() {}
CmdSD::~CmdSD() {}

void CmdSD::run(String** args, uint32_t argsLength) {
  if(argsLength < 1) {
    printf("--- HELP ---\n");
    printf("list: List all storage devices\n");
    printf("mount <device>: Mount a device\n");
    printf("unmount <device>: Unmount a device\n");
    return;
  }

  if(args[0]->equals("list")) {
    uint32_t devCount = PartitionManager::activePartitionManager->numDevices();
    printf("0x");
    printHex32(devCount);
    printf(" devices detected.\n");
    for(int i = 0; i < devCount; i++) {
      uint32_t partitions = PartitionManager::activePartitionManager->numPartitions(i);
      printf("Device 0x");
      printHex32(i);
      if(partitions == 0) {
        printf(" has no partitions!\n");
      } else {
        printf(": \n");
      }
      for(int p = 0; p < partitions; p++) {
        printf("  Partition 0x");
        printHex32(p);
        printf(" has 0x");
        printHex32(PartitionManager::activePartitionManager->getPartition(i, p)->getSectorCount());
        printf(" sectors!\n");
      }
    }
  } else if(args[0]->equals("mount")) {
    if(argsLength >= 2) {
      uint32_t deviceId = args[1]->parseInt();
      bool result = PartitionManager::activePartitionManager->mount(deviceId);
      printf("Device 0x");
      printHex32(deviceId);
      if(result) {
        uint32_t partitions = PartitionManager::activePartitionManager->numPartitions(deviceId);
        if(partitions == 0) {
          printf(" no partitions detected!\n");
        } else {
          printf(" mounted 0x");
          printHex32(partitions);
          printf(" partitions!\n");
        }
      } else {
        printf(" mount failed: ");
        if(PartitionManager::activePartitionManager->numPartitions(deviceId) != 0) {
          printf("already mounted!\n");
        } else {
          printf("device does not exist!\n");
        }
      }
    } else {
      printf("Missing device id!\n");
    }
  } else if(args[0]->equals("unmount")) {
    if(argsLength >= 2) {
      uint32_t deviceId = args[1]->parseInt();
      bool result = PartitionManager::activePartitionManager->unmount(deviceId);
      printf("Device 0x");
      printHex32(deviceId);
      if(result) {
        if(this->workingDirectory != 0) {
          delete this->workingDirectory;
          this->workingDirectory = 0;
        }
        printf(" unmounted!");
      } else {
        printf(" device is not mounted!");
      }
    } else {
      printf("Missing device id!\n");
    }
  } else {
    printf("Try sd without arguments for help\n");
  }
}
