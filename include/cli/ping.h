#ifndef __SYS__CLI__PING_H
#define __SYS__CLI__PING_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/icmp.h>

  namespace sys {
    namespace cli {
      class CmdPing : public Cmd {
      public:
        CmdPing();
        ~CmdPing();

        virtual void run(common::String** args, common::uint32_t argsLength);
      };
    }
  }

#endif