#ifndef __SYS__NET__ICMP_H
#define __SYS__NET__ICMP_H

  #include <common/types.h>
  #include <net/ipv4.h>
  
  namespace sys {
    namespace net {
      struct InternetControlMessageProtocolHeader {
        common::uint8_t type;
        common::uint8_t code;
        
        common::uint16_t checksum;
        common::uint32_t data;
      } __attribute__((packed));
      
      class InternetControlMessageProtocol : public InternetProtocolV4Handler {
      private:
        bool isWaiting;
        bool hasResponse;
        common::uint32_t responseIp;
      public:
        InternetControlMessageProtocol(InternetProtocolV4Provider* backend);
        ~InternetControlMessageProtocol();
        
        virtual bool onInternetProtocolReceived(common::uint32_t srcIp_BE, common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
        common::uint32_t ping(common::uint32_t ip_be, common::uint8_t ttl);
      };
    }
  }

#endif
