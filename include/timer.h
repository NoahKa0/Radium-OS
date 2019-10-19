#ifndef __SYS__TIMER_H
#define __SYS__TIMER_H

  #include <common/types.h>
  namespace sys {
    class SystemTimer {
    protected:
      common::uint64_t time;
    public:
      static SystemTimer* activeTimer;
      
      SystemTimer();
      ~SystemTimer();
      
      common::uint64_t getTimeInInterrupts();
      
      void onTimerInterrupt();
    };
  }

#endif
