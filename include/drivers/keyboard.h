#ifndef __SYS__DRIVERS__KEYBOARD_H
#define __SYS__DRIVERS__KEYBOARD_H

    #include <common/types.h>
    #include <drivers/driver.h>
    #include <hardware/interrupts.h>
    #include <hardware/port.h>
    
    namespace sys {
        namespace drivers {
            class KeyboardEventHandler {
            public:
                KeyboardEventHandler();
                ~KeyboardEventHandler();
                
                virtual void onKeyDown(char);
                virtual void onKeyUp(char);
            };
            
            class KeyboardDriver : public sys::hardware::InterruptHandler, public sys::drivers::Driver {
                sys::hardware::Port8Bit dataPort;
                sys::hardware::Port8Bit commandPort;
                
            public:
                KeyboardDriver(sys::hardware::InterruptManager* interruptManager, sys::drivers::KeyboardEventHandler* keyboardEventHandler);
                ~KeyboardDriver();
                
                KeyboardEventHandler* handler;
                
                virtual sys::common::uint32_t handleInterrupt(sys::common::uint32_t esp);
                virtual void activate();
            };
        }
    }


#endif
