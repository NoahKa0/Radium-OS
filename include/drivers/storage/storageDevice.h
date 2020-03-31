#ifndef __MYOS__DRIVERS__STORAGE__STORAGEDEVICE_H
#define __MYOS__DRIVERS__STORAGE__STORAGEDEVICE_H

  #include <common/types.h>
  #include <hardware/port.h>
  #include <hardware/pci.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace drivers {
      namespace storage {
        class StorageDevice:public Driver {
        public:
          StorageDevice();
          ~StorageDevice();
          
          virtual void identify();
          virtual void read(common::uint64_t sector, common::uint8_t* data, int count);
          virtual void write(common::uint64_t sector, common::uint8_t* data, int count);
          virtual void flush();

          virtual common::uint64_t getSectorCount();

          void findPartitions();
        };
      }
    }
  }
    
#endif