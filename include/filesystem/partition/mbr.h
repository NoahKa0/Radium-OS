#ifndef __SYS__FILESYSTEM__PARTITION__MBR_H
#define __SYS__FILESYSTEM__PARTITION__MBR_H

  #include <common/types.h>
  #include <filesystem/partition/partition.h>
  #include <filesystem/partition/partitionManager.h>
  #include <drivers/storage/storageDevice.h>

  namespace sys {
    namespace filesystem {
      namespace partition {

        struct MBRTableEntry {
          common::uint8_t bootable;

          common::uint8_t startHead;
          common::uint8_t startSector : 6;
          common::uint16_t startCylinder : 10;

          common::uint8_t partitionId;

          common::uint8_t endHead;
          common::uint8_t endSector : 6;
          common::uint16_t endCylinder : 10;

          common::uint32_t startLba;
          common::uint32_t length;
        } __attribute__((packed));

        struct MBRStructure {
          common::uint8_t bootloader[440];
          common::uint32_t signature;
          common::uint16_t unused;

          MBRTableEntry primaryPartition[4];

          common::uint16_t magicnumber;
        } __attribute__((packed));

        class MBR {
        public:
          static bool isMBR(drivers::storage::StorageDevice* device);
          static partitionEntry* findPartitions(drivers::storage::StorageDevice* device);
        };
      }
    }
  }

#endif