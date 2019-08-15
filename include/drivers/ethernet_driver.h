#ifndef __MYOS__DRIVERS__ETHERNET_DRIVER_H
#define __MYOS__DRIVERS__ETHERNET_DRIVER_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <hardware/interrupts.h>
  #include <net/etherframe.h>
  
  namespace sys {
    namespace net {
      class EtherFrameProvider;
    }
  }
  
  namespace sys {
    namespace drivers {
      class EthernetDriver : public Driver {
      private:
        char* defaultMessage;
      protected:
        net::EtherFrameProvider* etherFrameProvider;
      public:
        EthernetDriver();
        ~EthernetDriver();
        
        virtual void send(common::uint8_t* buffer, int size);
        virtual void receive();
        virtual common::uint64_t getMacAddress();
        
        void setEtherFrameProvider(net::EtherFrameProvider* efp);
      };
    }
  }
    
#endif
