#ifndef __MYOS__DRIVERS__AUDIO__AC97_H
#define __MYOS__DRIVERS__AUDIO__AC97_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <hardware/pci.h>
  #include <hardware/interrupts.h>
  #include <memorymanagement/memorymanagement.h>
  #include <hardware/port.h>
  #include <drivers/audio/AudioDriver.h>

  namespace sys {
    namespace drivers {
      namespace audio {
        struct AC97Desc {
          common::uint16_t* address;
          common::uint16_t length;
          common::uint16_t options;
        }  __attribute__((packed));

        class AC97 : public AudioDriver, public hardware::InterruptHandler {
        private:
          enum {
            GlobalControl = 0x2C,
            ReservedControlBits = 0x3 << 20,
            LinkShutoff = 1 << 3,
            WarmReset = 1 << 2,
            ColdReset = 1 << 1,
            GlobalInterruptEnable = 1,
            Sample20BitEnable = 1 << 22,

            In = 0,
            Out = 0x10,
            DescArrayAddr = 0x00,
            CurrentEntry = 0x04,
            LastValidEntry = 0x05,
            StatusRegister = 0x06,
            ProcessedSamples = 0x08,
            ProcessedEntry = 0x0A,
            BufferControl = 0x0B,
              BufferTransfer = 1,
              BufferReset = 1 << 1,
              BufferInterruptOnLast = 1 << 2,
              BufferInterruptOnComplete = 1 << 3,
              BufferInterruptOnError = 1 << 4,
            DescEntryLength = 0x8000,
            LastDescEntry = 1 << 14,
            IoC = 1 << 15,
            descArraySize = 32,
            Channels = 2
          };

          common::uint8_t* baseAddr;
          common::uint8_t* mixAddr;
          common::uint32_t basePort;
          common::uint32_t mixPort;
          sys::hardware::PeripheralComponentDeviceDescriptor* device;

          void csr8w(common::uint32_t reg, common::uint8_t value);
          void csr16w(common::uint32_t reg, common::uint16_t value);
          void csr32w(common::uint32_t reg, common::uint32_t value);

          common::uint8_t csr8r(common::uint32_t reg);
          common::uint16_t csr16r(common::uint32_t reg);
          common::uint32_t csr32r(common::uint32_t reg);

          sys::hardware::Port8Bit port8Bit;
          sys::hardware::Port16Bit port16Bit;
          sys::hardware::Port32Bit port32Bit;

          common::uint32_t writeEntry;
          common::uint32_t writePosition;

          common::uint32_t readEntry;

          common::uint8_t volume;

          bool active;

          AC97Desc* descriptorsOut;

          void start();
        public:
          AC97(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
          ~AC97();

          virtual bool isBigEndian();
          virtual common::uint8_t getChannels();
          virtual common::uint8_t getSampleSize();
          virtual void write(common::uint8_t* samples, common::uint32_t sizeInBytes);
          virtual common::uint32_t samplesReady(); // Amount of samples that can be written to the driver without blocking.
          virtual common::uint32_t preferedBufferSize();
          virtual void stop();
          
          virtual void setVolume(common::uint8_t volume);

          virtual void activate();
          virtual int reset();
          virtual void deactivate();
          
          virtual common::uint32_t handleInterrupt(common::uint32_t esp);

          static Driver* getDriver(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
        };
      }
    }
  }

#endif