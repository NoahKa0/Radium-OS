#ifndef __SYS__FILESYSTEM__PARTITION__PARTITION_H
#define __SYS__FILESYSTEM__PARTITION__PARTITION_H

  #include <common/types.h>
  #include <drivers/storage/storageDevice.h>

  namespace sys {
    namespace filesystem {
      namespace partition {
        class Partition {
        protected:
          common::uint64_t startLBA;
          common::uint64_t endLBA;
          common::uint64_t length;
          drivers::storage::StorageDevice* device;
        public:
          Partition(drivers::storage::StorageDevice* device, common::uint64_t startLBA, common::uint64_t length);
          ~Partition();

          void read(common::uint64_t sector, common::uint8_t* data, int length);
          void write(common::uint64_t sector, common::uint8_t* data, int length);

          common::uint64_t getSectorCount();
          drivers::storage::StorageDevice* getDevice();

          void findFileSystem();
        };
      }
    }
  }

#endif