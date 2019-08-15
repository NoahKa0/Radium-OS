#include <memorymanagement.h>

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
  MemoryChunk* result = 0;
  
  for(MemoryChunk* i = first; i != 0 && result == 0; i = i->next) {
    if(i->size > size && !i->allocated) {
      result = i;
    }
  }
  
  if(result == 0) {
    return 0;
  }
  
  result->allocated = true; // If multiple tasks are allocating memory the other task won't bother our current chunk.
  if(result->size > size+sizeof(MemoryChunk) + 1) {
    MemoryChunk* tmp = (MemoryChunk*)(size_t) (result + sizeof(MemoryChunk)+size);
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
  MemoryChunk* chunk = (MemoryChunk*)(size_t) (pointer - sizeof(MemoryChunk));
  
  chunk->allocated = false;
  
  if(chunk->previous != 0 && !chunk->previous->allocated) {
    chunk = chunk->previous;
  }
  
  for(MemoryChunk* i = chunk->next; i != 0; i = i->next) {
    if(!i->allocated) {
      chunk->size += i->size + sizeof(MemoryChunk);
    } else {
      i->previous = chunk;
      chunk->next = i;
      i = 0;
    }
  }
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
