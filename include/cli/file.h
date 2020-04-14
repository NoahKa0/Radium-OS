#ifndef __SYS__CLI__FILE_H
#define __SYS__CLI__FILE_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>

  namespace sys {
    namespace cli {
      class CmdFILE : public Cmd {
      public:
        CmdFILE();
        virtual ~CmdFILE();

        virtual void run(common::String** args, common::uint32_t argsLength);
      };
    }
  }

#endif