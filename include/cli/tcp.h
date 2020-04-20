#ifndef __SYS__CLI__TCP_H
#define __SYS__CLI__TCP_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/tcp.h>

  namespace sys {
    namespace cli {
      class CmdTCP : public Cmd {
      net::TransmissionControlProtocolSocket* socket;
      public:
        CmdTCP();
        ~CmdTCP();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };
    }
  }

#endif