#ifndef __SYS__NET__ETHERFRAME_H
#define __SYS__NET__ETHERFRAME_H

  #include <common/types.h>
  #include <drivers/net/ethernet_driver.h>
  
  namespace sys {
    namespace drivers {
      class EthernetDriver;
    }
  }
  
  namespace sys {
    namespace net {
      struct EtherFrameHeader {
        common::uint64_t destMac : 48;
        common::uint64_t srcMac : 48;
        common::uint16_t etherType;
      }  __attribute__((packed));
      
      typedef common::uint32_t EtherFrameFooter;
      
      class EtherFrameProvider;
      
      class EtherFrameHandler {
      protected:
        EtherFrameProvider* backend;
        common::uint16_t etherType;
      public:
        EtherFrameHandler(EtherFrameProvider* backend, common::uint16_t etherType);
        ~EtherFrameHandler();
        
        virtual bool onEtherFrameReceived(common::uint8_t* etherFramePayload, common::uint32_t size);
        virtual void send(common::uint64_t destMacAddress, common::uint8_t* etherFramePayload, common::uint32_t size);
        
        common::uint32_t getIpAddress();
        void setIpAddress(common::uint32_t ip);
        common::uint64_t getMacAddress();
      };
      
      class EtherFrameProvider {
        friend class EtherFrameHandler;
      protected:
        EtherFrameHandler* handlers[65535];
        drivers::EthernetDriver* ethernetDriver;
      public:
        EtherFrameProvider(drivers::EthernetDriver* backend);
        ~EtherFrameProvider();
        
        bool onRawDataRecived(common::uint8_t* buffer, common::uint32_t size);
        void send(common::uint64_t destMacAddress_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size);
        
        common::uint64_t getMacAddress();
        common::uint32_t getIpAddress();
        void setIpAddress(common::uint32_t ip);
      };
    }
  }

#endif
