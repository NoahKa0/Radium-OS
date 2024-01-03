#ifndef __SYS__CLI__TRACEROUTE_H
#define __SYS__CLI__TRACEROUTE_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/NetworkManager.h>

  namespace sys {
    namespace cli {
      class CmdTraceRoute : public Cmd {
      public:
        CmdTraceRoute();
        ~CmdTraceRoute();

        virtual void run(common::String** args, common::uint32_t argsLength);
      };
    }
  }

#endif