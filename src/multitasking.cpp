#include <multitasking.h>

using namespace sys;
using namespace sys::common;

/**
  Class: Task
**/
Task::Task(GlobalDescriptorTable* gdt, void entrypoint()) {
    cpuState = (CPUState*)(stack + 524288 - sizeof(CPUState));

    cpuState->eax = 0;
    cpuState->ebx = 0;
    cpuState->ecx = 0;
    cpuState->edx = 0;

    cpuState->esi = 0;
    cpuState->edi = 0;
    cpuState->ebp = 0;

    /*
    cpuState->gs = 0;
    cpuState->fs = 0;
    cpuState->es = 0;
    cpuState->ds = 0;
    */

    // cpuState->error = 0;    

    // cpuState->esp = ;
    cpuState->eip = (uint32_t)entrypoint;
    cpuState->cs = gdt->CodeSegmentSelector();
    // cpuState->ss = ;
    cpuState->eflags = 0x202;
}

Task::~Task() {}

/**
  Class: TaskManager
**/

TaskManager::TaskManager() {
  numTasks = 0;
  currentTask = -1;
  this->taskSwitchingEnabled = false;
}
TaskManager::~TaskManager() {}

bool TaskManager::addTask(Task* task) {
  if(numTasks >= 256) return false;
  
  tasks[numTasks] = task;
  numTasks++;
  return true;
}

CPUState* TaskManager::schedule(CPUState* cpuState) {
  if(numTasks <= 0 || !this->taskSwitchingEnabled) return cpuState; // No tasks, or task switching disabled, switch back.
  
  if(currentTask >= 0) {
    tasks[currentTask]->cpuState = cpuState;
  }
  currentTask++;
  if(currentTask >= numTasks) {
    currentTask = 0;
  }
  
  return tasks[currentTask]->cpuState;
}

void TaskManager::enable() {
  this->taskSwitchingEnabled = true;
}
