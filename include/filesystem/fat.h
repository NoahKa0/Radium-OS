#ifndef __SYS__FILESYSTEM__FAT_H
#define __SYS__FILESYSTEM__FAT_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/storage/storageDevice.h>

  namespace sys {
    namespace filesystem {

      struct fatBPB {
        common::uint8_t jump[3];
        common::uint8_t softname[8];
        common::uint16_t byesPerSector;
        common::uint8_t sectorsPerCluster;
        common::uint16_t reservedSectors;
        
        common::uint8_t fatCopies;
        common::uint16_t rootDirEntries;
        common::uint16_t totalSectors;
        common::uint8_t mediaType;
        common::uint16_t fatSectorCount;
        common::uint16_t sectorsPerTrack;
        common::uint16_t headCount;
        common::uint32_t hiddenSectors;
        common::uint32_t totalSectorCount;

        common::uint32_t tableSize;
        common::uint16_t extFlags;
        common::uint16_t fatVersion;
        common::uint32_t rootCluster;
        common::uint16_t fatInfo;
        common::uint16_t backupSector;
        common::uint32_t reserved[3];
        common::uint8_t driveNumber;
        common::uint8_t reserved1;
        common::uint8_t bootSignature;
        common::uint32_t volumeId;
        common::uint8_t volumeLabel[11];
        common::uint8_t fatTypeLabel[8];
      } __attribute__((packed));

      struct fatDirectoryEntry {
        common::uint8_t name[8];
        common::uint8_t extension[3];
        common::uint8_t attributes;
        common::uint8_t reserved;
        common::uint8_t createdTimeTenth;

        common::uint16_t createdTime;
        common::uint16_t createdDate;
        common::uint16_t accessedTime;

        common::uint16_t firstClusterHi;
        common::uint16_t writeTime;
        common::uint16_t writeDate;
        common::uint16_t firstClusterLow;
        common::uint32_t size;
      } __attribute__((packed));

      class fat {
      public:
        static void readBPB(drivers::storage::StorageDevice* ata, common::uint32_t offset);
      };

    }
  }

#endif