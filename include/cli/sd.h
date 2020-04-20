#ifndef __SYS__CLI__SD_H
#define __SYS__CLI__SD_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <filesystem/partition/partitionManager.h>

  namespace sys {
    namespace cli {
      class CmdSD : public Cmd {
      public:
        CmdSD();
        virtual ~CmdSD();

        virtual void run(common::String** args, common::uint32_t argsLength);
      };
    }
  }

#endif