#ifndef __SYS__AUDIO__AUDIO_H
#define __SYS__AUDIO__AUDIO_H

  #include <common/types.h>
  #include <common/string.h>
  #include <audio/AudioStream.h>

  namespace sys {
    namespace drivers {
      namespace audio {
        class AudioDriver;
      }
    }
  }

  namespace sys {
    namespace audio {
      class Audio {
      protected:
        drivers::audio::AudioDriver* driver;
      public:
        static Audio* activeAudioManager;

        Audio();
        ~Audio();

        void registerDriver(drivers::audio::AudioDriver* driver);
        AudioStream* getStream();
      };
    }
  }

#endif