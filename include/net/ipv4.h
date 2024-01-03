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
        common::uint16_t flagsAndFragmentOffset;
        
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
        
        virtual bool onInternetProtocolReceived(common::uint32_t srcIp_BE, common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
        virtual void send(common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size, common::uint8_t ttl = 0x40);
      };
      
      class InternetProtocolV4Provider : public EtherFrameHandler {
      friend class InternetProtocolV4Handler;
      protected:
        InternetProtocolV4Handler* handlers[255];
        AddressResolutionProtocol* arp;
        common::uint32_t gateway;
        common::uint32_t subnetMask;
        common::uint32_t domainServer;
      public:
        InternetProtocolV4Provider(EtherFrameProvider* backend, AddressResolutionProtocol* arp);
          
        ~InternetProtocolV4Provider();
        
        virtual bool onEtherFrameReceived(common::uint8_t* etherFramePayload, common::uint32_t size);
        virtual void send(common::uint32_t destIpAddress, common::uint8_t protocol, common::uint8_t* etherFramePayload, common::uint32_t size, common::uint8_t ttl);
        
        AddressResolutionProtocol* getArp();
        
        static common::uint16_t checksum(common::uint16_t* data, common::uint32_t size);
        
        void setSubnetmask(common::uint32_t subnetmask);
        void setGateway(common::uint32_t gateway);
        void setDomainServer(common::uint32_t domainServer);
        common::uint32_t getDomainServer();
      };
    }
  }

#endif
