#ifndef __MYOS__DRIVERS__AUDIO__AUDIO_DRIVER_H
#define __MYOS__DRIVERS__AUDIO__AUDIO_DRIVER_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <hardware/interrupts.h>
  
  namespace sys {
    namespace drivers {
      namespace audio {
        class AudioDriver : public Driver {
        public:
          AudioDriver();
          virtual ~AudioDriver();

          virtual void setVolume(common::uint8_t volume); // Volume where 255 = 100%.

          virtual bool isBigEndian();
          virtual common::uint8_t getChannels(); // Number of channels.
          virtual common::uint8_t getSampleSize(); // Sample size 1 = 8 bit samples, 2 = 16 bit samples, etc.
          virtual void write(common::uint8_t* samples, common::uint32_t sizeInBytes);
          virtual common::uint32_t samplesReady(); // Amount of samples that can safely be written to the driver.
        };
      }
    }
  }
    
#endif
