#ifndef __SYS__MEMORYMANAGEMENT_H
#define __SYS__MEMORYMANAGEMENT_H

  #include <common/types.h>
  namespace sys {
    struct MemoryChunk {
      MemoryChunk* previous;
      MemoryChunk* next;
      bool allocated;
      common::size_t size;
    };
    
    class MemoryManager {
    protected:
      MemoryChunk* first;
    public:
      static MemoryManager* activeMemoryManager;
      
      MemoryManager(common::size_t start, common::size_t size);
      ~MemoryManager();
      void* malloc(common::size_t size);
      void free(void* chunk);
    };
  }
  
  void* operator new(sys::common::size_t size);
  void* operator new[](sys::common::size_t size);
  
  void* operator new(sys::common::size_t size, void* pointer);
  void* operator new[](sys::common::size_t size, void* pointer);
  
  void operator delete(void* pointer);
  void operator delete[](void* pointer);

#endif
