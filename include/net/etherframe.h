#ifndef __SYS__NET__ETHERFRAME_H
#define __SYS__NET__ETHERFRAME_H

  #include <common/types.h>
  #include <drivers/ethernet_driver.h>
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
        
        bool onEtherFrameReceived(common::uint8_t* etherFramePayload, common::uint32_t size);
        void send(common::uint64_t destMacAddress, common::uint8_t* etherFramePayload, common::uint32_t size);
      };
      
      class EtherFrameProvider {
      protected:
        EtherFrameHandler* handlers[65535];
        EthernetDriver* ethernetDriver;
      public:
        EtherFrameProvider(EthernetDriver* backend);
        ~EtherFrameProvider();
        
        bool onRawDataRecived(common::uint8_t* buffer, common::uint32_t size);
        void send(common::uint64_t destMacAddress, common::uint16_t etherType, common::uint8_t* buffer, common::uint32_t size);
      };
    }
  }

#endif
