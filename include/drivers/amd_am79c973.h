#ifndef __MYOS__DRIVERS__AMD_AM79C973_H
#define __MYOS__DRIVERS__AMD_AM79C973_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <hardware/interrupts.h>
  #include <hardware/pci.h>
  #include <hardware/port.h>
  #include <memorymanagement.h>
  
  namespace sys {
    namespace drivers {
      class amd_am79c973 : public Driver, public hardware::InterruptHandler {
      private:
        struct InitializationBlock {
          common::uint16_t mode;
          unsigned reserved1 : 4;
          unsigned numSendBuffers : 4;
          unsigned reserved2 : 4;
          unsigned numReciveBuffers : 4;
          common::uint64_t physicalAddress : 48;
          common::uint16_t reserved3;
          common::uint64_t logicalAddress;
          common::uint32_t reciveBufferDescrAddress;
          common::uint32_t sendBufferDescrAddress;
        } __attribute__((packed));
        
        struct BufferDescriptor {
          common::uint32_t address;
          common::uint32_t flags;
          common::uint32_t flags2;
          common::uint32_t available;
        } __attribute__((packed));
        
        hardware::Port16 macAddress8Port;
        hardware::Port16 macAddress4Port;
        hardware::Port16 macAddress2Port;
        
        hardware::Port16 registerDataPort;
        hardware::Port16 registerAddressPort;
        
        hardware::Port16 resetPort;
        
        hardware::Port16 busControlRegisterDataPort;
        
        InitializationBlock initBlock;
        
        BufferDescriptor* sendBuffersDesc;
        common::uint8_t sendBuffersDescMemory[2*1024+15];
        common::uint8_t sendBuffers[2*1024+15][8];
        common::uint8_t currentSendBuffer;
        
        BufferDescriptor* reciveBuffersDesc;
        common::uint8_t reciveBuffersDescMemory[2*1024+15];
        common::uint8_t reciveBuffers[2*1024+15][8];
        common::uint8_t currentReciveBuffer;
        
      public:
        amd_am79c973(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
        ~amd_am79c973();
        
        virtual void activate();
        virtual int reset();
        
        virtual common::uint32_t handleInterrupt(common::uint32_t esp);
      };
    }
  }
    
#endif
