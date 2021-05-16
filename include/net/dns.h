#ifndef __SYS__NET__DNS_H
#define __SYS__NET__DNS_H

  #include <common/types.h>
  #include <common/string.h>
  #include <net/tcp/TransmissionControlProtocolProvider.h>
  #include <net/ipv4.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace net {
      class DomainNameSystem {
      private:
        struct CacheDescriptor {
          common::String* name;
          common::uint32_t ip;
          common::uint16_t identifier;
          common::uint64_t discardAfter;
          bool answered;
        } __attribute__((packed));

        struct DnsHeader {
          common::uint16_t id;
          common::uint16_t flags;
          common::uint16_t questionCount;
          common::uint16_t answerCount;
          common::uint16_t nsCount; // I will not use this.
          common::uint16_t additionalCount;
        } __attribute__((packed));
      
        tcp::TransmissionControlProtocolProvider* tcpProvider;

        common::uint32_t attempts;
        
        common::uint16_t lastIdentifier;
        common::uint32_t cacheSize;
        common::uint32_t cachePointer;
        CacheDescriptor* cache;

        void askServer(common::String* domainName, common::uint32_t domainServer, CacheDescriptor* cacheDescriptor);
      public:
        DomainNameSystem(tcp::TransmissionControlProtocolProvider* tcpProvider);
        ~DomainNameSystem();
        
        common::uint32_t resolve(common::String* domainName, common::uint32_t domainServer);
        void sendRequest(common::String* domainName, tcp::TransmissionControlProtocolSocket* socket, common::uint16_t id);
        CacheDescriptor* addToCache(common::uint16_t id, common::String* domainName);
        common::uint32_t parseAnswer(tcp::TransmissionControlProtocolSocket* socket, CacheDescriptor* cacheDescriptor);
        common::uint32_t getLabelLength(common::uint8_t* label, common::uint32_t maxLength);
      };
    }
  }

#endif
