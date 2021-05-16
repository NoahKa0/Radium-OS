#ifndef __SYS__NET__TCP__SOCKET_H
#define __SYS__NET__TCP__SOCKET_H

  #include <common/types.h>
  #include <memorymanagement/memorymanagement.h>
  #include <timer.h>
  
  namespace sys {
    namespace net {
      namespace tcp {
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
        
        struct TransmissionControlProtocolPacket {
          common::uint8_t* data;
          common::uint32_t length;
          common::uint32_t sequenceNumber;
          
          common::uint64_t lastTransmit;
          common::uint32_t size;
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

          common::uint8_t sizeUnprocessedPackets;
          TransmissionControlProtocolPacket** unprocessedPackets;
          
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
          bool addRecvPacket(TransmissionControlProtocolPacket* packet, common::uint32_t dataLength);
          
          void removeOldPackets(common::uint32_t acknum);
          TransmissionControlProtocolPacket* getSendPacket(common::uint16_t n);
          void deleteSendPacket(common::uint16_t n);
          bool addSendPacket(TransmissionControlProtocolPacket* packet);

          bool addUnprocessedPacket(TransmissionControlProtocolPacket* packet);
          TransmissionControlProtocolPacket* getUnprocessedPacket();
          
        public:
          TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend);
          ~TransmissionControlProtocolSocket();
          
          virtual void send(common::uint8_t* data, common::uint32_t length);
          
          void disconnect();
          bool isConnected();
          bool isClosed();
          
          common::uint32_t hasNext();
          void readNext(common::uint8_t* data, common::uint32_t length);
          
          void sendExpiredPackets();
        };
      }
    }
  }

#endif
