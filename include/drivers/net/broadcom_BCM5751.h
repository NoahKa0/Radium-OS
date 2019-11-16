#ifndef __MYOS__DRIVERS__BROADCOM_BCM5751_H
#define __MYOS__DRIVERS__BROADCOM_BCM5751_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <drivers/net/ethernet_driver.h>
  #include <hardware/interrupts.h>
  #include <hardware/pci.h>
  #include <hardware/port.h>
  #include <memorymanagement.h>
  
  namespace sys {
    namespace drivers {
      class broadcom_BCM5751 : public EthernetDriver, public hardware::InterruptHandler {
      private:
        
      public:
        broadcom_BCM5751(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
        ~broadcom_BCM5751();
        
        virtual void activate();
        virtual int reset();
        
        virtual common::uint32_t handleInterrupt(common::uint32_t esp);
        
        virtual void send(common::uint8_t* buffer, int size);
        virtual void receive();
        virtual common::uint64_t getMacAddress();
        
        virtual common::uint32_t getIpAddress();
        virtual void setIpAddress(common::uint32_t ip);
      };
    }
  }
    
#endif
