#ifndef __SYS__NET__UDP__PROVIDER_H
#define __SYS__NET__UDP__PROVIDER_H

  #include <common/types.h>
  #include <net/ipv4.h>
  #include <net/udp/UserDatagramProtocolSocket.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace net {
      namespace udp {
        struct UserDatagramProtocolHeader {
          common::uint16_t srcPort;
          common::uint16_t destPort;
          
          common::uint16_t length;
          common::uint16_t checksum;
        } __attribute__((packed));
        
        class UserDatagramProtocolProvider : public InternetProtocolV4Handler {
        protected:
          UserDatagramProtocolSocket* sockets[65535];
          common::uint16_t numSockets;
          common::uint16_t freePort;
        public:
          UserDatagramProtocolProvider(InternetProtocolV4Provider* backend);
          ~UserDatagramProtocolProvider();
          
          virtual bool onInternetProtocolReceived(common::uint32_t srcIp_BE, common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
          virtual void sendUDP(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint16_t length);
          
          UserDatagramProtocolSocket* connect(common::uint32_t ip, common::uint16_t port, common::uint16_t localPort = 0);
          void disconnect(UserDatagramProtocolSocket* socket);
        };
      }
    }
  }

#endif
