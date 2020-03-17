#ifndef __SYS__PAGEMANAGEMENT_H
#define __SYS__PAGEMANAGEMENT_H

  #include <common/types.h>
  namespace sys {
    class PageManager {
    private:
      bool inKernel;
      common::uint32_t* kernelDictionary;
    public:
      static PageManager* activePageManager;

      PageManager();
      ~PageManager();

      /*
      * @return pointer to kernel page dictionary.
      */
      common::uint32_t* getKernelPageDictionary();

      /*
      * Creates a new page dictionary.
      * 
      * @return pointer to page dictionary.
      */
      common::uint32_t* createPageDictionary();
    };
  }

#endif
