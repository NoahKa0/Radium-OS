#ifndef __SYS__CLI__WAUDIO_H
#define __SYS__CLI__WAUDIO_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/NetworkManager.h>
  #include <net/http.h>
  #include <audio/Audio.h>
  #include <audio/AudioStream.h>

  namespace sys {
    namespace cli {
      class CmdWAUDIO : public Cmd {
      private:
        bool shouldStop;
      public:
        CmdWAUDIO();
        virtual ~CmdWAUDIO();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };
    }
  }

#endif