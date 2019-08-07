#ifndef __MYOS__DRIVERS__ATA_H
#define __MYOS__DRIVERS__ATA_H

  #include <common/types.h>
  #include <hardware/port.h>
  #include <memorymanagement.h>
  
  namespace sys {
    namespace drivers {
      class AdvancedTechnologyAttachment {
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
        bool master;
      public:
        AdvancedTechnologyAttachment(common::uint16_t portBase, bool master);
        ~AdvancedTechnologyAttachment();
        
        void identify();
        void read28(common::uint32_t sector, common::uint8_t* data, int count);
        void write28(common::uint32_t sector, common::uint8_t* data, int count);
        void flush();
      };
    }
  }
    
#endif
