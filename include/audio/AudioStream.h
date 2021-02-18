#ifndef __SYS__AUDIO__AUDIOSTREAM_H
#define __SYS__AUDIO__AUDIOSTREAM_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/audio/AudioDriver.h>

  namespace sys {
    namespace audio {
      class Audio;

      class AudioStream {
      friend class Audio;
      protected:
        drivers::audio::AudioDriver* driver;
        bool isBigEndian;

        AudioStream(drivers::audio::AudioDriver* driver);
      public:
        ~AudioStream();

        void setVolume(common::uint8_t volume);
        void write(common::uint16_t* samples, common::uint32_t sizeInBytes);
        void setBigEndian(bool isBE);
      };
    }
  }

#endif