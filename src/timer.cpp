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

uint16_t SystemTimer::readTime() {
  uint16_t ret = 0;
  
  this->command.write(0b00000000); // might need to be 0b11000000 we'll see.
  ret = this->channel0.read();
  ret |= (this->channel0.read() << 8) & 0xFF00;
  
  return ret;
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

void SystemTimer::sleep(uint32_t milliseconds) {
  if(SystemTimer::activeTimer == 0) return;
  
  uint32_t sleepTime = (milliseconds*SystemTimer::activeTimer->frequency)/1000;
  uint32_t currentTime = SystemTimer::getTimeInInterrupts();
  while(SystemTimer::getTimeInInterrupts() < currentTime+sleepTime) {
    asm("hlt");
  }
}

void SystemTimer::sleepWithIntDisabled(uint32_t milliseconds) {
  if(SystemTimer::activeTimer == 0) return;
  
  SystemTimer* timer = SystemTimer::activeTimer;
  // PIT runs at 1193181 herz, since we want milliseconds, divide by 1000.
  uint64_t sleepTime = (milliseconds*1193);
  uint64_t expired = 0;
  uint16_t currentRead = ~timer->readTime(); // Invert so it ascents.
  uint16_t newRead = 0;
  
  while(expired <= sleepTime) {
    newRead = ~timer->readTime(); // Invert so it ascents.
    if(newRead < currentRead) {
      expired += newRead + (~currentRead & 0xFFFF); // When newRead is smaller there was a wrap around. So invert currentRead so i get the number until max.
    } else {
      expired += (newRead - currentRead);
    }
    currentRead = newRead;
  }
}
