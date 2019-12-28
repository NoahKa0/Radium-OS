#ifndef __SYS__HARDWARE__PCI_H
#define __SYS__HARDWARE__PCI_H

  #include <common/types.h>
  #include <hardware/port.h>
  #include <hardware/interrupts.h>
  #include <drivers/driver.h>
  
  namespace sys {
    namespace hardware {
      enum BaseAddressRegisterType {
          memoryMapping = 0,
          inputOutput = 1
      };
      
      class BaseAddressRegister {
      public:
        bool prefetchable;
        sys::common::uint8_t* address;
        sys::common::uint32_t size;
        BaseAddressRegisterType type;
      };
      
      class PeripheralComponentInterconnect; // Declare PeripheralComponentInterconnect, because we need it in the class below.
      class PeripheralComponentDeviceDescriptor {
      private:
        PeripheralComponentInterconnect* pci;
      public:
        sys::common::uint32_t portBase;
        sys::common::uint32_t interrupt;

        sys::common::uint16_t bus;
        sys::common::uint16_t device;
        sys::common::uint16_t function;

        sys::common::uint16_t vendorId;
        sys::common::uint16_t deviceId;

        sys::common::uint8_t classId;
        sys::common::uint8_t subclassId;
        sys::common::uint8_t interfaceId;

        sys::common::uint8_t revision;
        
        bool memoryMapped;

        PeripheralComponentDeviceDescriptor(PeripheralComponentInterconnect* pci);
        ~PeripheralComponentDeviceDescriptor();
        
        sys::common::uint32_t read(sys::common::uint32_t registerOffset);
        void write(sys::common::uint32_t registerOffset, sys::common::uint32_t value);
      };
      
      class PeripheralComponentInterconnect {
        sys::hardware::Port32Bit dataPort;
        sys::hardware::Port32Bit commandPort;
      public:
        PeripheralComponentInterconnect();
        ~PeripheralComponentInterconnect();
        
        sys::common::uint32_t read(sys::common::uint16_t bus, sys::common::uint16_t device, sys::common::uint16_t function, sys::common::uint32_t registerOffset);
        
        void write(sys::common::uint16_t bus, sys::common::uint16_t device, sys::common::uint16_t function, sys::common::uint32_t registerOffset, sys::common::uint32_t value);
        
        bool hasFunctions(sys::common::uint16_t bus, sys::common::uint16_t device);
        
        void selectDrivers(sys::drivers::DriverManager* driverManager, sys::hardware::InterruptManager* interruptManager);
        
        PeripheralComponentDeviceDescriptor getDeviceDescriptor(sys::common::uint16_t bus, sys::common::uint16_t device, sys::common::uint16_t function);
        
        BaseAddressRegister getBaseAddressRegister(sys::common::uint16_t bus, sys::common::uint16_t device, sys::common::uint16_t function, sys::common::uint16_t bar);
        
        sys::drivers::Driver* getDriver(PeripheralComponentDeviceDescriptor dev, sys::hardware::InterruptManager* interruptManager);
      };
    }
  }
  
#endif
