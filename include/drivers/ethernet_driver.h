#ifndef __MYOS__DRIVERS__ETHERNET_DRIVER_H
#define __MYOS__DRIVERS__ETHERNET_DRIVER_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <hardware/interrupts.h>
  
  namespace sys {
    namespace drivers {
      class EthernetDriver : public Driver {
      private:
        char* defaultMessage;
      public:
        EthernetDriver();
        ~EthernetDriver();
        
        virtual void send(common::uint8_t* buffer, int size);
        virtual void receive();
      };
    }
  }
    
#endif
