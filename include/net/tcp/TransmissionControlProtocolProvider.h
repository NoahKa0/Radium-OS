#ifndef __SYS__NET__TCP__PROVIDER_H
#define __SYS__NET__TCP__PROVIDER_H

  #include <common/types.h>
  #include <net/ipv4.h>
  #include <net/tcp/TransmissionControlProtocolSocket.h>
  #include <memorymanagement/memorymanagement.h>
  #include <timer.h>
  
  namespace sys {
    namespace net {
      namespace tcp {        
        struct TransmissionControlProtocolHeader {
          common::uint16_t srcPort;
          common::uint16_t destPort;
          
          common::uint32_t sequenceNumber;
          common::uint32_t acknowledgementNumber;
          
          common::uint8_t reserved : 4;
          common::uint8_t headerSize32 : 4;
          common::uint8_t flags;
          
          common::uint16_t windowSize;
          common::uint16_t checksum;
          common::uint16_t urgent;
          
          common::uint32_t options;
        } __attribute__((packed));
        
        struct TransmissionControlProtocolPseudoHeader {
          common::uint32_t srcIp;
          common::uint32_t destIp;
          
          common::uint16_t protocol;
          common::uint16_t totalLength;
        } __attribute__((packed));
        
        class TransmissionControlProtocolProvider : public InternetProtocolV4Handler {
        protected:
          TransmissionControlProtocolSocket* sockets[65535];
          common::uint16_t numSockets;
          common::uint16_t freePort;
          
          common::uint32_t generateSequenceNumber();
        public:
          TransmissionControlProtocolProvider(InternetProtocolV4Provider* backend);
          ~TransmissionControlProtocolProvider();
          
          bool processData(TransmissionControlProtocolSocket* socket, common::uint8_t* payload, TransmissionControlProtocolHeader* header, common::uint32_t size);
          virtual bool onInternetProtocolReceived(common::uint32_t srcIp_BE, common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
          virtual void sendTCP(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t length, common::uint16_t flags = 0, bool noRetransmit = false);
          
          void sendExpiredPackets(TransmissionControlProtocolSocket* socket);
          
          TransmissionControlProtocolSocket* listen(common::uint16_t port);
          TransmissionControlProtocolSocket* connect(common::uint32_t ip, common::uint16_t port, common::uint16_t localPort = 0);
          void disconnect(TransmissionControlProtocolSocket* socket);
          
          void removeSocket(TransmissionControlProtocolSocket* socket);
        };
      }
    }
  }

#endif
