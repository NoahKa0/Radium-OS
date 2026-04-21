#ifndef __SYS__CLI__COLOR_H
#define __SYS__CLI__COLOR_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/NetworkManager.h>
  #include <net/http.h>
  #include <drivers/vga.h>

  namespace sys {
    namespace cli {
      class CmdColor : public Cmd {
      public:
        CmdColor();
        ~CmdColor();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };
    }
  }

#endif