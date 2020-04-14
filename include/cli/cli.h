#ifndef __SYS__CLI__CLI_H
#define __SYS__CLI__CLI_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/keyboard.h>
  #include <net/icmp.h>
  #include <net/tcp.h>
  #include <filesystem/partition/partitionManager.h>

  namespace sys {
    namespace cli {
      class Cmd {
      public:
        Cmd();
        virtual ~Cmd();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };

      class Cli : public drivers::KeyboardEventHandler {
      private:
        char* cmdBuffer;
        common::uint32_t cmdBufferSize;
        common::uint32_t cmdBufferPos;
        common::String* command;
        Cmd* cmd;
        bool running;
      public:
        Cli();
        ~Cli();

        virtual void onKeyDown(char);

        void run();
      };
    }
  }

#endif
