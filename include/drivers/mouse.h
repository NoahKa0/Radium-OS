#ifndef __SYS__DRIVERS__MOUSE_H
#define __SYS__DRIVERS__MOUSE_H

    #include <common/types.h>
    #include <drivers/driver.h>
    #include <hardware/interrupts.h>
    #include <hardware/port.h>
    
    namespace sys {
        namespace drivers {
            
            class MouseEventHandler {
            public:
                MouseEventHandler();
                ~MouseEventHandler();
                
                virtual void onMouseDown();
                virtual void onMouseUp();
                
                virtual void onMove(int x, int y);
            };
            
            class MouseDriver : public sys::hardware::InterruptHandler, public sys::drivers::Driver {
                sys::hardware::Port8Bit dataPort;
                sys::hardware::Port8Bit commandPort;
                
                sys::common::uint8_t buffer[3];
                sys::common::uint8_t offset;
                sys::common::uint8_t buttons;
                
                MouseEventHandler* handler;
            public:
                MouseDriver(sys::hardware::InterruptManager* interruptManager, sys::drivers::MouseEventHandler* handler);
                ~MouseDriver();
                
                virtual sys::common::uint32_t handleInterrupt(sys::common::uint32_t esp);
                virtual void activate();
            };
        }
    }


#endif
