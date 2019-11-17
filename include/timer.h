#ifndef __SYS__TIMER_H
#define __SYS__TIMER_H

  #include <common/types.h>
  #include <hardware/port.h>
  
  namespace sys {
    class SystemTimer {
    protected:
      common::uint64_t time;
      common::uint32_t frequency;
      
      hardware::Port8Bit channel0;
      hardware::Port8Bit channel1;
      hardware::Port8Bit channel2;
      hardware::Port8Bit command;
      
      common::uint16_t readTime();
    public:
      static SystemTimer* activeTimer;
      
      SystemTimer();
      ~SystemTimer();
      
      void onTimerInterrupt();
      
      static common::uint64_t getTimeInInterrupts();
      
      static void sleep(common::uint32_t milliseconds);
      
      static void sleepWithIntDisabled(common::uint32_t milliseconds);
    };
  }

#endif
