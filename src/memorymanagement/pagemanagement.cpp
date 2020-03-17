#include <memorymanagement/pagemanagement.h>
#include <memorymanagement/memorymanagement.h>

using namespace sys;
using namespace sys::common;

PageManager* PageManager::activePageManager = 0;

void printf(char*);
void printHex32(uint32_t);

PageManager::PageManager() {
  activePageManager = this;

  this->inKernel = true;

  // Allocate both the dictionary and all tables, since we will fill them all anyway.
  this->kernelDictionary = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(sizeof(uint32_t) * 1025 * 1024, 4096);

  // Skip over dictionary
  uint32_t* tables = this->kernelDictionary + 1024;

  uint32_t record;
  // Fill dictionary
  for(uint32_t i = 0; i < 1024; i++) {
    record = (uint32_t) tables;
    // First 18 bits are 4 KiB aligned address to table.
    // Set Read/Write enable and present (last two bits).
    record = 3 | (0xFFFFF000 & (record + 1024 * sizeof(uint32_t) * i));
    this->kernelDictionary[i] = record;
  }

  // Fill tables
  for(uint32_t i = 0; i < 1024*1024; i++) {
    // Addresses are 4 KiB aligned, so left shift 12 bits and add the same flags as in the dictionary.
    record = ((i << 12) & 0xFFFFF000) | 3;
    tables[i] = record;
  }

  uint32_t dictionary = (uint32_t) this->kernelDictionary;
  
  /**
  * Load address of dictionary in cr3.
  * Set first bit (paging) and last bit (protected mode) of cr0. Protected mode is already on, but paging requires it, so i'm doing it again to make that clear.
  */
  __asm__( 
    "  mov %0, %%eax\n"
    "  mov %%eax, %%cr3\n"
    "  mov %%cr0, %%eax\n"
    "  or $0x80000001, %%eax\n"
    "  mov %%eax, %%cr0\n"
    ::"r" (dictionary) /* %0 */
  );
}

PageManager::~PageManager() {}

common::uint32_t* PageManager::getKernelPageDictionary() {
  return this->kernelDictionary;
}

common::uint32_t* PageManager::createPageDictionary() {
  // Kernel takes up first 4 page tables.
  uint32_t* dictionary = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(sizeof(uint32_t) * 5 * 1024, 4096);
  uint32_t* tables = dictionary + 1024;

  uint32_t record;
  for(uint32_t i = 0; i < 1024; i++) {
    if(i < 4) { // Pages for kernel.
      record = (uint32_t) tables;
      // Set Read/Write enable and present. User should not be set (only kernel has access to these pages).
      record = 3 | (0xFFFFF000 & (record + 1024 * sizeof(uint32_t) * i));
    } else {
      // Set Read/Write enable and User. Present bit will be off.
      record = 6;
      this->kernelDictionary[i] = record;
    }
  }
}
