#ifndef __SYS__CLI__AUDIO_H
#define __SYS__CLI__AUDIO_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <audio/Audio.h>
  #include <audio/AudioStream.h>

  namespace sys {
    namespace cli {
      class CmdAUDIO : public Cmd {
      public:
        CmdAUDIO();
        virtual ~CmdAUDIO();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);

        void changeWorkingDirectory(filesystem::File* file);
      };
    }
  }

#endif