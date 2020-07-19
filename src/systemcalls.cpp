#include <systemcalls.h>

using namespace sys;
using namespace sys::common;
using namespace sys::hardware;

SystemCallHandler::SystemCallHandler(InterruptManager* interruptManager)
: InterruptHandler(0x80, interruptManager) {}

SystemCallHandler::~SystemCallHandler() {}

void printf(const char* str);

uint32_t SystemCallHandler::handleInterrupt(sys::common::uint32_t esp) {
  CPUState* cpuState = (CPUState*) esp;
  
  switch(cpuState->eax) {
    case 4:
      printf((char*) cpuState->ebx);
      break;
    default:
      break;
  }
  
  return esp;
}
