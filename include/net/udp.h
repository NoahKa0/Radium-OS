#ifndef __SYS__NET__UDP_H
#define __SYS__NET__UDP_H

  #include <common/types.h>
  #include <net/ipv4.h>
  #include <memorymanagement.h>
  
  namespace sys {
    namespace net {
      struct UserDatagramProtocolHeader {
        common::uint16_t srcPort;
        common::uint16_t destPort;
        
        common::uint16_t length;
        common::uint16_t checksum;
      } __attribute__((packed));
      
      class UserDatagramProtocolProvider;
      class UserDatagramProtocolSocket;
      
      
      class UserDatagramProtocolHandler {
      public:
        UserDatagramProtocolHandler();
        ~UserDatagramProtocolHandler();
        virtual void handleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint32_t length);
      };
      
      class UserDatagramProtocolSocket {
      friend class UserDatagramProtocolProvider;
      protected:
        common::uint16_t localPort;
        common::uint32_t localIp;
        common::uint16_t remotePort;
        common::uint32_t remoteIp;
        
        bool forwardBroadcasts;
        bool forwardAll;
        
        UserDatagramProtocolProvider* backend;
        UserDatagramProtocolHandler* handler;
      public:
        UserDatagramProtocolSocket(UserDatagramProtocolProvider* backend);
        ~UserDatagramProtocolSocket();
        
        virtual void handleUserDatagramProtocolMessage(common::uint8_t* data, common::uint32_t length);
        virtual void send(common::uint8_t* data, common::uint16_t length);
        void setHandler(UserDatagramProtocolHandler* handler);
        void disconnect();
        
        void enableForwardAll();
        void enableBroadcasts();
        void setDestinationIp(common::uint32_t ip);
      };
      
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
        
        UserDatagramProtocolSocket* connect(common::uint32_t ip, common::uint16_t port);
        UserDatagramProtocolSocket* connect(common::uint32_t ip, common::uint16_t port, common::uint16_t localPort);
        void disconnect(UserDatagramProtocolSocket* socket);
      };
    }
  }

#endif
