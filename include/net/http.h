#ifndef __SYS__NET__HTTP_H
#define __SYS__NET__HTTP_H

  #include <common/types.h>
  #include <common/string.h>
  #include <net/tcp/TransmissionControlProtocolProvider.h>
  #include <net/ipv4.h>
  #include <memorymanagement/memorymanagement.h>
  
  namespace sys {
    namespace net {
      class Http {
      private:
        tcp::TransmissionControlProtocolSocket* socket;
        common::uint32_t responseCode;
        common::uint32_t responseSize;
        
        void getResponse();
        void processLine(char* line);
      public:
        Http(tcp::TransmissionControlProtocolSocket* socket);
        ~Http();
        
        void request(common::String* host, common::String* query);
        common::uint32_t getResponseCode();
        common::uint32_t getContentLength();
      };
    }
  }

#endif
