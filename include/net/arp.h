#ifndef __SYS__NET__ARP_H
#define __SYS__NET__ARP_H

  #include <common/types.h>
  #include <net/etherframe.h>
  
  namespace sys {
    namespace net {
      struct AddressResolutionProtocolMessage {
        common::uint16_t hardwareType;
        common::uint16_t protocol;
        common::uint8_t hardwareAddressLength; // MAC = 6.
        common::uint8_t protocolAddressLength; // IPv4 = 4.
        
        common::uint16_t command;
        common::uint64_t senderMacAddress : 48;
        common::uint32_t senderIpAddress;
        common::uint64_t destMacAddress : 48;
        common::uint32_t destIpAddress;
        
      } __attribute__((packed));
      
      class AddressResolutionProtocol : public EtherFrameHandler {
      protected:
        common::uint32_t ipCache[128];
        common::uint64_t macCache[128];
        common::uint32_t currentCache;
      public:
        AddressResolutionProtocol(EtherFrameProvider* backend);
        ~AddressResolutionProtocol();
        
        bool onEtherFrameReceived(common::uint8_t* etherFramePayload, common::uint32_t size);
        
        void requestMacAddress(common::uint32_t ip_BE);
        common::uint64_t getMacFromCache(common::uint32_t ip_BE);
        
        common::uint64_t resolve(common::uint32_t ip_BE);
      };
    }
  }

#endif
