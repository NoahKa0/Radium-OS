#ifndef __SYS__TEST__CLI_H
#define __SYS__TEST__CLI_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/keyboard.h>
  #include <net/icmp.h>
  #include <net/tcp.h>

  namespace sys {
    namespace test {
      class Cmd {
      public:
        Cmd();
        ~Cmd();

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

      class CmdPing : public Cmd {
      public:
        CmdPing();
        ~CmdPing();

        virtual void run(common::String** args, common::uint32_t argsLength);
      };

      class CmdTCP : public Cmd {
      net::TransmissionControlProtocolSocket* socket;
      public:
        CmdTCP();
        ~CmdTCP();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };

      class CmdIMG : public Cmd {
      private:
        common::uint8_t x;
        common::uint8_t y;
        common::uint8_t r;
        common::uint8_t g;
        common::uint8_t b;
      public:
        CmdIMG();
        ~CmdIMG();

        virtual void run(common::String** args, common::uint32_t argsLength);
      };

    }
  }

#endif
