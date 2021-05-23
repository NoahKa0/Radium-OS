#include <timer.h>

using namespace sys;
using namespace sys::common;

/**
  This timer should be improved, it assumes that the PIT is configured correctly by the BIOS (~18,2 interrupts per second).
  I think this is bad, and i should make a driver that controlls the frequency and handles the interrupts.
  But since i need a simple timer for TCP and generation of random numbers, this will do for now.
**/

SystemTimer* SystemTimer::activeTimer = 0;

SystemTimer::SystemTimer()
:channel0(0x40),
channel1(0x41),
channel2(0x42),
command(0x43)
{
  activeTimer = this;
  
  this->time = 0;
  this->frequency = 18;
}

SystemTimer::~SystemTimer() {
  if(activeTimer == this) {
    activeTimer = 0;
  }
}

void SystemTimer::onTimerInterrupt() {
  this->time++;
}

uint64_t SystemTimer::getTimeInInterrupts() {
  if(SystemTimer::activeTimer != 0) {
    return SystemTimer::activeTimer->time;
  }
  return 0;
}

uint64_t SystemTimer::getFrequency() {
  if(SystemTimer::activeTimer != 0) {
    return SystemTimer::activeTimer->frequency;
  }
  return 1;
}

uint32_t SystemTimer::millisecondsToLength(common::uint32_t milliseconds) {
  if(SystemTimer::activeTimer == 0) return 0;
  
  return (milliseconds*SystemTimer::activeTimer->frequency)/1000;
}

void SystemTimer::sleep(uint32_t milliseconds) {
  if(SystemTimer::activeTimer == 0) return;
  
  uint32_t sleepTime = (milliseconds*SystemTimer::activeTimer->frequency)/1000;
  uint32_t currentTime = SystemTimer::getTimeInInterrupts();
  while(SystemTimer::getTimeInInterrupts() < currentTime+sleepTime) {
    asm("hlt");
  }
}
