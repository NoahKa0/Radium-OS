#ifndef __SYS__PAGEMANAGEMENT_H
#define __SYS__PAGEMANAGEMENT_H

  #include <common/types.h>
  namespace sys {
    class PageManager {
    private:
      bool inKernel;
      common::uint32_t* kernelDictionary;
      common::uint32_t** pageDictionaries;

    public:
      static PageManager* activePageManager;

      PageManager();
      ~PageManager();

      /*
      * Switches to the context of the kernel.
      */
      void switchToKernel();

      /*
      * Switches to the context of the provided page id.
      */
      bool contextSwitch(common::int16_t pageId);

      /*
      * Creates a new page dictionary.
      * 
      * @param startSize - size of memory reserved for all the contents of the pages, this does not include the size needed for the kernel.
      * 
      * @return pageId
      */
      common::uint16_t createPageDictionary(common::uint32_t startSize);
    };
  }

#endif
