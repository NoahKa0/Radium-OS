#ifndef __SYS__CLI__CLI_H
#define __SYS__CLI__CLI_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/keyboard.h>
  #include <filesystem/filesystem.h>

  namespace sys {
    namespace cli {
      class Cli;

      class Cmd {
      friend class Cli;
      protected:
        static filesystem::File* workingDirectory;
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
        void showCommandInput();
      };
    }
  }

#endif
