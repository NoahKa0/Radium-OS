#ifndef __SYS__NET__UDP__SOCKET_H
#define __SYS__NET__UDP__SOCKET_H

  #include <common/types.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace net {
      namespace udp {
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
      }
    }
  }

#endif
