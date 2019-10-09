#ifndef __SYS__NET__TCP_H
#define __SYS__NET__TCP_H

  #include <common/types.h>
  #include <net/ipv4.h>
  #include <memorymanagement.h>
  
  namespace sys {
    namespace net {
      enum TransmissionControlProtocolSocketState {
        CLOSED,
        LISTEN,
        SYN_SENT,
        SYN_RECEIVED,
        ESTABLISHED,
        FIN_WAIT1,
        FIN_WAIT2,
        CLOSING,
        TIME_WAIT,
        CLOSE_WAIT,
      };
      
      enum TransmissionControlProtocolFlag {
        FIN = 1,
        SYN = 2,
        RST = 4,
        PSH = 8,
        ACK = 16,
        URG = 32,
        ECE = 64,
        CWR = 128,
        NS = 256
      };
      
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
      
      class TransmissionControlProtocolProvider;
      class TransmissionControlProtocolSocket;
      
      
      class TransmissionControlProtocolHandler {
      public:
        TransmissionControlProtocolHandler();
        ~TransmissionControlProtocolHandler();
        virtual bool handleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint32_t length);
      };
      
      class TransmissionControlProtocolSocket {
      friend class TransmissionControlProtocolProvider;
      protected:
        common::uint16_t localPort;
        common::uint32_t localIp;
        common::uint16_t remotePort;
        common::uint32_t remoteIp;
        
        common::uint32_t sequenceNumber;
        common::uint32_t acknowledgementNumber;
        
        TransmissionControlProtocolProvider* backend;
        TransmissionControlProtocolHandler* handler;
        
        TransmissionControlProtocolSocketState state;
      public:
        TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend);
        ~TransmissionControlProtocolSocket();
        
        virtual bool handleTransmissionControlProtocolMessage(common::uint8_t* data, common::uint32_t length);
        virtual void send(common::uint8_t* data, common::uint16_t length);
        void setHandler(TransmissionControlProtocolHandler* handler);
        void disconnect();
        bool isClosed();
      };
      
      class TransmissionControlProtocolProvider : public InternetProtocolV4Handler {
      protected:
        TransmissionControlProtocolSocket* sockets[65535];
        common::uint16_t numSockets;
        common::uint16_t freePort;
        
        common::uint32_t generateSequenceNumber();
      public:
        TransmissionControlProtocolProvider(InternetProtocolV4Provider* backend);
        ~TransmissionControlProtocolProvider();
        
        virtual bool onInternetProtocolReceived(common::uint32_t srcIp_BE, common::uint32_t destIp_BE, common::uint8_t* payload, common::uint32_t size);
        virtual void sendTCP(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t length, common::uint16_t flags = 0);
        
        TransmissionControlProtocolSocket* listen(common::uint16_t port);
        TransmissionControlProtocolSocket* connect(common::uint32_t ip, common::uint16_t port, common::uint16_t localPort = 0);
        void disconnect(TransmissionControlProtocolSocket* socket);
      };
    }
  }

#endif
