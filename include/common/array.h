#ifndef __SYS__COMMON__ARRAY_H
#define __SYS__COMMON__ARRAY_H

#include <common/types.h>

namespace sys {
  namespace common {
    class Array {
    private:
      common::uint8_t** items;
      uint32_t length;
      uint32_t bufferLength;
      bool autoclean;
    public:
      Array(bool autoclean);
      ~Array();

      uint32_t getLength();
      uint8_t* get(common::uint32_t n);
      void set(common::uint32_t n, common::uint8_t* data);
      void add(common::uint8_t* data);
      void remove(common::uint32_t n);
    };
  }
}

#endif
