#ifndef __SYS__DRIVERS__VGA_H
#define __SYS__DRIVERS__VGA_H

    #include <common/types.h>
    #include <drivers/driver.h>
    #include <hardware/port.h>
    
    namespace sys {
        namespace drivers {
            class VideoGraphicsArray {
            protected:
                sys::hardware::Port8Bit miscPort;
                
                sys::hardware::Port8Bit ctrlIndexPort;
                sys::hardware::Port8Bit ctrlDataPort;
                
                sys::hardware::Port8Bit sequencerIndexPort;
                sys::hardware::Port8Bit sequencerDataPort;
                
                sys::hardware::Port8Bit graphicsControllerIndexPort;
                sys::hardware::Port8Bit graphicsControllerDataPort;
                
                sys::hardware::Port8Bit attributeControllerIndexPort;
                sys::hardware::Port8Bit attributeControllerReadPort;
                sys::hardware::Port8Bit attributeControllerWritePort;
                sys::hardware::Port8Bit attributeControllerResetPort;
                
                void writeRegisters(sys::common::uint8_t* registers);
                
                sys::common::uint8_t* getFrameBufferSegment();
                
                virtual void putPixel(sys::common::uint32_t x, sys::common::uint32_t y, sys::common::uint8_t color);
                virtual sys::common::uint8_t getColorIndex(sys::common::uint8_t r, sys::common::uint8_t g, sys::common::uint8_t b);
            public:
                VideoGraphicsArray();
                ~VideoGraphicsArray();
                virtual bool supportsMode(sys::common::uint32_t width, sys::common::uint32_t height, sys::common::uint32_t colorDepth);
                virtual bool setMode(sys::common::uint32_t width, sys::common::uint32_t height, sys::common::uint32_t colorDepth);
                virtual void putPixel(sys::common::uint32_t x, sys::common::uint32_t y, sys::common::uint8_t r, sys::common::uint8_t g, sys::common::uint8_t b);
            };
        }
    }


#endif
