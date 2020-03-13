#ifndef __SYS__COMMON__STRING_H
#define __SYS__COMMON__STRING_H

#include <common/types.h>

namespace sys {
  namespace common {
    class String {
    private:
      char* chars;
      uint32_t length;
    public:
      String();
      String(char* str);
      ~String();

      uint32_t getLength();
      uint32_t occurrences(char c);
      String* split(char c, uint32_t occurrence);
      String* substring(uint32_t start, uint32_t end = 0);
      uint32_t strpos(char c, uint32_t occurrence = 0);
      char* getCharPtr();
      int64_t parseInt();
      bool contains(char* str, uint32_t strlen = 0);
      bool equals(char* str);
      void append(char* str);
      static uint32_t findLength(char* str);
    };
  }
}

#endif
