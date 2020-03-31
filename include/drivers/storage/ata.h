#ifndef __MYOS__DRIVERS__STORAGE__ATA_H
#define __MYOS__DRIVERS__STORAGE__ATA_H

  #include <drivers/storage/storageDevice.h>
  #include <common/types.h>
  #include <hardware/port.h>
  #include <hardware/pci.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace drivers {
      namespace storage {
        class AdvancedTechnologyAttachment:public StorageDevice {
        protected:
          hardware::Port16Bit dataPort;
          hardware::Port8Bit errorPort;
          hardware::Port8Bit sectorCountPort;
          hardware::Port8Bit lbaLowPort;
          hardware::Port8Bit lbaMidPort;
          hardware::Port8Bit lbaHiPort;
          hardware::Port8Bit devicePort;
          hardware::Port8Bit commandPort;
          hardware::Port8Bit controlPort;
          
          common::uint16_t bytesPerSector;
          common::uint64_t sectorCount;
          bool master;
        public:
          AdvancedTechnologyAttachment(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
          ~AdvancedTechnologyAttachment();

          virtual void activate();
          
          virtual void identify();
          virtual void read(common::uint64_t sector, common::uint8_t* data, int count);
          virtual void write(common::uint64_t sector, common::uint8_t* data, int count);
          virtual void flush();

          virtual common::uint64_t getSectorCount();
        };
      }
    }
  }
    
#endif
