#include <timer.h>

using namespace sys;
using namespace sys::common;

/**
  This timer should be improved, it assumes that the PIT is configured correctly by the BIOS (~18,2 interrupts per second).
  I think this is bad, and i should make a driver that controlls the frequency and handles the interrupts.
  But since i need a simple timer for TCP and generation of random numbers, this will do for now.
**/

SystemTimer* SystemTimer::activeTimer = 0;

SystemTimer::SystemTimer() {
  activeTimer = this;
  
  this->time = 0;
}

SystemTimer::~SystemTimer() {
  if(activeTimer == this) {
    activeTimer = 0;
  }
}

uint64_t SystemTimer::getTimeInInterrupts() {
  if(SystemTimer::activeTimer != 0) {
    return SystemTimer::activeTimer->time;
  }
  return 0;
}

void SystemTimer::onTimerInterrupt() {
  this->time++;
}
