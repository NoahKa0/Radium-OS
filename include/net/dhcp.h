#ifndef __SYS__NET__DHCP_H
#define __SYS__NET__DHCP_H

  #include <common/types.h>
  #include <net/udp.h>
  #include <net/ipv4.h>
  #include <memorymanagement.h>
  
  namespace sys {
    namespace net {
      struct DynamicHostConfigurationProtocolHeader {
        common::uint8_t operationCode;
        common::uint8_t htype;
        common::uint8_t hlength;
        common::uint8_t hops;
        
        common::uint32_t identifier;
        
        common::uint16_t secs;
        common::uint16_t flags;
        
        common::uint32_t clientAddress;
        common::uint32_t myAddress;
        common::uint32_t serverAddress;
        common::uint32_t gatewayAddress;
        
        common::uint64_t clientHardwareAddress1;
        common::uint64_t clientHardwareAddress2;
        
        common::uint8_t serverName[64];
        common::uint8_t bootFileName[128];
        
        common::uint32_t optionsMagicCookie;
        common::uint8_t options[308];
      } __attribute__((packed));
      
      struct DynamicHostConfigurationOptionsInfo {
        common::uint32_t messageType;
        common::uint32_t ip;
        common::uint32_t subnetmask;
        common::uint32_t defaultGateway;
        common::uint32_t lease;
      } __attribute__((packed));
      
      class DynamicHostConfigurationProtocol : public UserDatagramProtocolHandler {
      private:
        UserDatagramProtocolSocket* socket;
        InternetProtocolV4Provider* ipv4;
        
        common::uint32_t magicCookie;
        
        common::uint32_t ip;
        common::uint32_t subnetmask;
        common::uint32_t defaultGateway;
        common::uint32_t lease;
        common::uint32_t ip_firstOffer;
        
        common::uint32_t identifier;
        
        DynamicHostConfigurationOptionsInfo* getOptionsInfo(common::uint8_t* options, common::uint32_t size);
        void handleOption(DynamicHostConfigurationOptionsInfo* info, common::uint8_t* option, common::uint8_t code, common::uint32_t size);
        
      public:
        DynamicHostConfigurationProtocol(UserDatagramProtocolProvider* udp, InternetProtocolV4Provider* ipv4);
        ~DynamicHostConfigurationProtocol();
        
        virtual void handleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint32_t length);
        
        void sendDiscover();
        void handleOffer(DynamicHostConfigurationProtocolHeader* message, DynamicHostConfigurationOptionsInfo* info);
        void sendRequest(DynamicHostConfigurationProtocolHeader* offer);
        void handleAck(DynamicHostConfigurationProtocolHeader* message, DynamicHostConfigurationOptionsInfo* info, bool nak);
      };
    }
  }

#endif
