#ifndef __SYS__MEMORYMANAGEMENT_H
#define __SYS__MEMORYMANAGEMENT_H

  #include <common/types.h>
  namespace sys {
    struct MemoryChunk {
      MemoryChunk* previous;
      MemoryChunk* next;
      bool allocated;
      common:size_t size;
    };
    
    class MemoryManager {
    protected:
      MemoryChunk* first;
    public:
      static MemoryManager* activeMemoryManager;
      
      MemoryManager(common:size_t first, common:size_t size);
      ~MemoryManager();
      void* malloc(size_t size);
      void free(void* chunk);
    };
  }

#endif