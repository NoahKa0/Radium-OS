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
      String(const char* str, common::uint32_t length = 0);
      ~String();

      uint32_t getLength();
      uint32_t occurrences(char c);
      String* split(char c, uint32_t occurrence);
      String* substring(uint32_t start, uint32_t end = 0);
      String* trim(char c);
      uint32_t strpos(char c, uint32_t occurrence = 0);
      char* getCharPtr();
      char charAt(uint32_t index);
      int64_t parseInt();
      bool contains(const char* str, uint32_t strlen = 0);
      bool equals(const char* str);
      void append(const char* str);
      void toLower();
      void toUpper();
      static uint32_t findLength(const char* str, common::uint32_t max = 0);
    };
  }
}

#endif
