#ifndef __SYS__FILESYSTEM__PARTITION__PARTITIONMANAGER_H
#define __SYS__FILESYSTEM__PARTITION__PARTITIONMANAGER_H

  #include <common/types.h>
  #include <common/string.h>
  #include <filesystem/partition/partition.h>
  #include <drivers/storage/storageDevice.h>

  namespace sys {
    namespace filesystem {
      namespace partition {
        struct partitionEntry {
          Partition* partition;
          partitionEntry* next;
        };
        struct deviceEntry {
          drivers::storage::StorageDevice* device;
          partitionEntry* partition;
        };
        class PartitionManager {
        private:
          deviceEntry* devices[32];
          common::uint32_t deviceCount;

          partitionEntry* findPartitions(drivers::storage::StorageDevice* device);
        public:
          static PartitionManager* activePartitionManager;

          PartitionManager();
          ~PartitionManager();

          void registerDevice(drivers::storage::StorageDevice* device);

          common::uint32_t numDevices();
          drivers::storage::StorageDevice* getDevice(common::uint32_t device);

          bool mount(common::uint32_t device);
          bool unmount(common::uint32_t device);
          
          common::uint32_t numPartitions(common::uint32_t device);
          Partition* getPartition(common::uint32_t device, common::uint32_t partition);

          common::uint8_t* getFullFile(common::String* filepath);
        };
      }
    }
  }

#endif