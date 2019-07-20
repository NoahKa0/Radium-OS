#ifndef __SYS__HARDWARE__PORT_H
#define __SYS__HARDWARE__PORT_H
    
#include <common/types.h>

    namespace sys {
        namespace hardware {
            class Port {
            protected:
                sys::common::uint16_t portnumber;
                Port(sys::common::uint16_t portnumber);
                ~Port();
            };
            
            class Port8Bit : public Port {
            public:
                Port8Bit(sys::common::uint16_t portnumber);
                ~Port8Bit();
                virtual void write(sys::common::uint8_t data);
                virtual sys::common::uint8_t read();
            };
            
            class Port8BitSlow : public Port8Bit {
            public:
                Port8BitSlow(sys::common::uint16_t portnumber);
                ~Port8BitSlow();
                virtual void write(sys::common::uint8_t data);
            };
            
            class Port16Bit : public Port {
            public:
                Port16Bit(sys::common::uint16_t portnumber);
                ~Port16Bit();
                virtual void write(sys::common::uint16_t data);
                virtual sys::common::uint16_t read();
            };
            
            class Port32Bit : public Port {
            public:
                Port32Bit(sys::common::uint16_t portnumber);
                ~Port32Bit();
                virtual void write(sys::common::uint32_t data);
                virtual sys::common::uint32_t read();
            };
        }
    }

#endif
