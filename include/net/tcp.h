#ifndef __SYS__NET__TCP_H
#define __SYS__NET__TCP_H

  #include <common/types.h>
  #include <net/ipv4.h>
  #include <memorymanagement.h>
  #include <timer.h>
  
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
      
      struct TransmissionControlProtocolPacket {
        common::uint8_t* data;
        common::uint32_t length;
        common::uint32_t sequenceNumber;
        
        common::uint64_t lastTransmit;
      } __attribute__((packed));
      
      class TransmissionControlProtocolProvider;
      
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
        
        TransmissionControlProtocolSocketState state;
        
        common::uint32_t nextBytes;
        
        common::uint8_t* recvBufferPtr;
        common::uint16_t recvBufferSize;
        common::uint16_t recvBufferPosition;
        common::uint16_t recvBufferReadPacket;
        common::uint16_t recvBufferReadPosition;
        
        common::uint8_t* sendBufferPtr;
        common::uint16_t sendBufferSize;
        common::uint16_t sendBufferPosition;
        
        common::uint64_t lastRecivedTime;
        
        common::uint16_t packetSize;
        
        TransmissionControlProtocolPacket* getRecvPacket(common::uint16_t n);
        void deleteRecvPacket(common::uint16_t n);
        bool addRecvPacket(TransmissionControlProtocolPacket* packet);
        
        void removeOldPackets(common::uint32_t acknum);
        TransmissionControlProtocolPacket* getSendPacket(common::uint16_t n);
        void deleteSendPacket(common::uint16_t n);
        bool addSendPacket(TransmissionControlProtocolPacket* packet);
        
      public:
        TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend);
        ~TransmissionControlProtocolSocket();
        
        virtual void send(common::uint8_t* data, common::uint32_t length);
        
        void disconnect();
        bool isConnected();
        bool isClosed();
        
        common::uint32_t hasNext();
        void readNext(common::uint8_t* data, common::uint32_t length);
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
        virtual void sendTCP(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t length, common::uint16_t flags = 0, bool noRetransmit = false);
        
        void sendExpiredPackets(TransmissionControlProtocolSocket* socket);
        
        TransmissionControlProtocolSocket* listen(common::uint16_t port);
        TransmissionControlProtocolSocket* connect(common::uint32_t ip, common::uint16_t port, common::uint16_t localPort = 0);
        void disconnect(TransmissionControlProtocolSocket* socket);
        
        void removeSocket(TransmissionControlProtocolSocket* socket);
      };
    }
  }

#endif
