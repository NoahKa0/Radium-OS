#ifndef __SYS__DRIVERS__VGA_H
#define __SYS__DRIVERS__VGA_H

    #include <common/types.h>
    #include <drivers/driver.h>
    #include <hardware/port.h>
    
    namespace sys {
        namespace drivers {
            class VideoGraphicsArray {
            protected:
                hardware::Port8Bit miscPort;
                
                hardware::Port8Bit ctrlIndexPort;
                hardware::Port8Bit ctrlDataPort;
                
                hardware::Port8Bit sequencerIndexPort;
                hardware::Port8Bit sequencerDataPort;
                
                hardware::Port8Bit graphicsControllerIndexPort;
                hardware::Port8Bit graphicsControllerDataPort;
                
                hardware::Port8Bit attributeControllerIndexPort;
                hardware::Port8Bit attributeControllerReadPort;
                hardware::Port8Bit attributeControllerWritePort;
                hardware::Port8Bit attributeControllerResetPort;
                
                void writeRegisters(uint8_t* registers);
                
                uint8_t* getFrameBufferSegment();
                
                virtual void putPixel(uint32_t x, uint32_t y, uint8_t color);
                virtual uint8_t getColorIndex(uint8_t r, uint8_t g, uint8_t b);
            public:
                VideoGraphicsArray();
                virtual ~VideoGraphicsArray();
                virtual bool supportsMode(uint32_t width, uint32_t height, uint32_t colorDepth);
                virtual bool setMode(uint32_t width, uint32_t height, uint32_t colorDepth);
                virtual void putPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
            };
        }
    }


#endif
