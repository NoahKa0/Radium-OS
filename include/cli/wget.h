#ifndef __SYS__CLI__WGET_H
#define __SYS__CLI__WGET_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/NetworkManager.h>
  #include <net/http.h>

  namespace sys {
    namespace cli {
      class CmdWGET : public Cmd {
      private:
        net::tcp::TransmissionControlProtocolSocket* socket;
        filesystem::File* appendFile;
        bool enteredFile;
        common::uint32_t pressedEnter;
      public:
        CmdWGET();
        ~CmdWGET();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };
    }
  }

#endif