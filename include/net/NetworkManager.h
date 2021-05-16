#ifndef __SYS__NET__NETWORKMANAGER_H
#define __SYS__NET__NETWORKMANAGER_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/net/ethernet_driver.h>
  #include <net/etherframe.h>
  #include <net/arp.h>
  #include <net/ipv4.h>
  #include <net/icmp.h>
  #include <net/udp/UserDatagramProtocolProvider.h>
  #include <net/tcp/TransmissionControlProtocolProvider.h>
  #include <net/dhcp.h>
  #include <net/dns.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace net {      
      class NetworkManager {
      private:
        EtherFrameProvider* etherframe;
        AddressResolutionProtocol* arp;
        InternetProtocolV4Provider* ipv4;
        InternetControlMessageProtocol* icmp;
        udp::UserDatagramProtocolProvider* udp;
        DynamicHostConfigurationProtocol* dhcp;
        tcp::TransmissionControlProtocolProvider* tcp;
        DomainNameSystem* dns;

        drivers::EthernetDriver* currentDriver;
        drivers::EthernetDriver* fallbackDriver;

        common::uint32_t parseIp(common::String* ip);
        common::uint32_t stringToIp(common::String* ip);
        common::uint32_t resolveIp(common::String* ip);
      public:
        static NetworkManager* networkManager;

        NetworkManager();
        ~NetworkManager();
        
        void registerEthernetDriver(drivers::EthernetDriver* drv);
        void setup();

        void ping(common::String* dest);
        tcp::TransmissionControlProtocolSocket* connectTCP(common::String* dest, common::uint16_t port, common::uint16_t localPort = 0);
        udp::UserDatagramProtocolSocket* connectUDP(common::String* dest, common::uint16_t port, common::uint16_t localPort = 0);

        bool hasLink();
        bool foundDriver();

        common::String* getIpString(common::String* dest);
      };
    }
  }

#endif
