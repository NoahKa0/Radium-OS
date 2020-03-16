#include <memorymanagement/memorymanagement.h>

using namespace sys;
using namespace sys::common;

MemoryManager* MemoryManager::activeMemoryManager = 0;

MemoryManager::MemoryManager(size_t start, size_t size) {
  activeMemoryManager = this;
  
  if(size < sizeof(MemoryChunk)) {
    first = 0;
  } else {
    first = (MemoryChunk*) start;
    first->previous = 0;
    first->next = 0;
    first->allocated = false;
    first->size = size - sizeof(MemoryChunk);
  }
}

MemoryManager::~MemoryManager() {}

void* MemoryManager::malloc(size_t size) {
  return mallocalign(size, 0);
}

void printf(char*);
void printHex32(uint32_t);

void* MemoryManager::mallocalign(size_t size, size_t alignment) {
  MemoryChunk* result = 0;
  
  for(MemoryChunk* i = first; i != 0 && result == 0; i = i->next) {
    if(i->size > size + alignment && !i->allocated) {
      result = i;
    }
  }
  
  if(result == 0) {
    printf("Out of memory!\nPANIC!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
    return 0;
  }
  
  result->allocated = true;

  size_t alignmentError = (size_t)result + sizeof(MemoryChunk);
  if(alignment != 0) { // n % 0, throws an error.
    alignmentError %= alignment;
  } else {
    alignmentError = 0;
  }

  // When alignment doesn't match, the whole chunk should be moved forward.
  if(alignmentError != 0 && result != first) {
    size_t diff = alignment - alignmentError;
    size_t size = result->size - diff;
    MemoryChunk* prev = result->previous;
    MemoryChunk* next = result->next;

    result += diff;
    result->allocated = true;
    result->next = next;
    result->previous = prev;
    result->size = size;
    
    if(next != 0) next->previous = result;
    prev->next = result;
    prev->size += diff; // Alignment is asually small enough that it isn't worth checking if a new chunk can be created.
  }

  if(result->size > size+sizeof(MemoryChunk) + 1) {
    MemoryChunk* tmp = (MemoryChunk*) ((size_t) result + sizeof(MemoryChunk)+size);
    tmp->allocated = false;
    tmp->size = result->size-sizeof(MemoryChunk)-size;
    tmp->previous = result;
    tmp->next = result->next;
    
    result->size = size;
    if(result->next != 0) {
      result->next->previous = tmp;
    }
    result->next = tmp;
  }

  return (void*) (((size_t) result) + sizeof(MemoryChunk));
}

void MemoryManager::free(void* pointer) {
  MemoryChunk* chunk = (MemoryChunk*) ((size_t) pointer - sizeof(MemoryChunk));
  
  chunk->allocated = false;
  
  if(chunk->previous != 0 && !chunk->previous->allocated) {
    chunk = chunk->previous;
  }
  
  MemoryChunk* i = chunk->next;
  while(i != 0 && !i->allocated) {
    chunk->size += i->size + sizeof(MemoryChunk);
    i = i->next;
  }
  i->previous = chunk;
  chunk->next = i;
}

void* operator new(unsigned size) {
  if(sys::MemoryManager::activeMemoryManager == 0) return 0;
  
  return sys::MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new[](unsigned size) {
  if(sys::MemoryManager::activeMemoryManager == 0) return 0;
  
  return sys::MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new(unsigned size, void* pointer) {
  return pointer;
}

void* operator new[](unsigned size, void* pointer) {
  return pointer;
}

void operator delete(void* pointer) {
  if(sys::MemoryManager::activeMemoryManager != 0) {
    sys::MemoryManager::activeMemoryManager->free(pointer);
  }
}
void operator delete[](void* pointer) {
  if(sys::MemoryManager::activeMemoryManager != 0) {
    sys::MemoryManager::activeMemoryManager->free(pointer);
  }
}

void operator delete(void* pointer, uint32_t size) {
  if(sys::MemoryManager::activeMemoryManager != 0) {
    sys::MemoryManager::activeMemoryManager->free(pointer);
  }
}

void operator delete[](void* pointer, uint32_t size) {
  if(sys::MemoryManager::activeMemoryManager != 0) {
    sys::MemoryManager::activeMemoryManager->free(pointer);
  }
}
