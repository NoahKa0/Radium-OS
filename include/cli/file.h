#ifndef __SYS__CLI__FILE_H
#define __SYS__CLI__FILE_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <filesystem/partition/partitionManager.h>

  namespace sys {
    namespace cli {
      class CmdFILE : public Cmd {
      private:
        filesystem::File* appendFile;
      public:
        CmdFILE();
        virtual ~CmdFILE();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);

        void changeWorkingDirectory(filesystem::File* file);
      };
    }
  }

#endif