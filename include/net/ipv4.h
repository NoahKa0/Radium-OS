#ifndef __SYS__NET__IPV4_H
#define __SYS__NET__IPV4_H

  #include <common/types.h>
  #include <net/etherframe.h>
  #include <net/arp.h>
  
  namespace sys {
    namespace net {
      struct InternetProtocolV4Message {
        common::uint8_t headerLength : 4;
        common::uint8_t version : 4;
        common::uint8_t tos; // Type Of Service.
        common::uint16_t length;
        
        common::uint16_t identification;
        common::uint8_t flags : 3;
        common::uint16_t fragmentOffset : 13;
        
        common::uint8_t timeToLive;
        common::uint8_t protocol;
        common::uint16_t checksum;
        
        common::uint32_t srcAddress;
        common::uint32_t destAddress;
        
      } __attribute__((packed));
      
      class InternetProtocolV4Provider;
      
      class InternetProtocolV4Handler {
      protected:
        InternetProtocolV4Provider* backend;
        common::uint8_t ipProtocol;
      public:
        InternetProtocolV4Handler(InternetProtocolV4Provider* backend, common::uint8_t ipProtocol);
        ~InternetProtocolV4Handler();
        
        bool onInternetProtocolReceived(common::uint32_t srcIp_BE, common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
        void send(common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
      };
      
      class InternetProtocolV4Provider : public EtherFrameHandler {
      friend class InternetProtocolV4Handler;
      protected:
        InternetProtocolV4Handler* handlers[255];
        AddressResolutionProtocol* arp;
        common::uint32_t gateway;
        common::uint32_t subnetMask;
      public:
        InternetProtocolV4Provider(EtherFrameProvider* backend, AddressResolutionProtocol* arp,
        common::uint32_t gateway, common::uint32_t subnetMask);
          
        ~InternetProtocolV4Provider();
        
        virtual bool onEtherFrameReceived(common::uint8_t* etherFramePayload, common::uint32_t size);
        virtual void send(common::uint32_t destIpAddress, common::uint8_t protocol, common::uint8_t* etherFramePayload, common::uint32_t size);
        
        static common::uint16_t checksum(common::uint16_t* data, common::uint32_t size);
      };
    }
  }

#endif
