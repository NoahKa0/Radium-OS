#ifndef __SYS__SYSTEMCALLS_H
#define __SYS__SYSTEMCALLS_H

    #include <common/types.h>
    #include <hardware/interrupts.h>
    #include <multitasking.h>
    namespace sys {
      class SystemCallHandler : public hardware::InterruptHandler {
      public:
        SystemCallHandler(hardware::InterruptManager* interruptManager);
        ~SystemCallHandler();
        
        virtual sys::common::uint32_t handleInterrupt(sys::common::uint32_t esp);
      };
    }

#endif
