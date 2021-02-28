#ifndef __SYS__AUDIO__AUDIOSTREAM_H
#define __SYS__AUDIO__AUDIOSTREAM_H

  #include <common/types.h>
  #include <common/string.h>
  #include <drivers/audio/AudioDriver.h>
  #include <memorymanagement/memorymanagement.h>

  namespace sys {
    namespace audio {
      class Audio;

      class AudioStream {
      friend class Audio;
      protected:
        drivers::audio::AudioDriver* driver;
        bool isBigEndian;
        common::uint32_t writePosition;
        common::uint32_t bufferSize;
        common::uint16_t* buffer;

        AudioStream(drivers::audio::AudioDriver* driver);
      public:
        ~AudioStream();

        void setVolume(common::uint8_t volume);
        void write(common::uint8_t* samples, common::uint32_t sizeInBytes);
        void flush();
        void stop();
        void setBigEndian(bool isBE);
      };
    }
  }

#endif