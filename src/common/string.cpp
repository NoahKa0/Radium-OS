#include <common/string.h>
#include <memorymanagement/memorymanagement.h>

using namespace sys;
using namespace sys::common;

String::String() {
  this->chars = (char*) MemoryManager::activeMemoryManager->malloc(sizeof(char));
  this->chars[0] = 0;
  this->length = 0;
}

String::String(char* str) {
  this->length = String::findLength(str);
  this->chars = (char *)MemoryManager::activeMemoryManager->malloc(sizeof(char) * (this->length + 1));
  for(int i = 0; i < this->length; i++) {
    this->chars[i] = str[i];
  }
  this->chars[this->length] = 0;
}

String::~String() {
  MemoryManager::activeMemoryManager->free(this->chars);
}

uint32_t String::getLength() {
  return this->length;
}

uint32_t String::occurrences(char c) {
  uint32_t ret = 0;
  for(int i = 0; i < this->length; i++) {
    if(this->chars[i] == c) {
      ret++;
    }
  }
  return ret;
}

String* String::split(char c, uint32_t occurrence) {
  uint32_t firstPos = 0;
  if(occurrence != 0) {
    firstPos = this->strpos(c, occurrence - 1) + 1;
  }
  uint32_t secondPos = this->strpos(c, occurrence);

  if(firstPos > secondPos) { // Not found
    return 0;
  }

  if(firstPos == secondPos) { // No chars between the two positions.
    return new String();
  }

  char secondPosChar = this->chars[secondPos];
  this->chars[secondPos] = 0;
  String* ret = new String(&this->chars[firstPos]);
  this->chars[secondPos] = secondPosChar;

  return ret;
}

String* String::substring(uint32_t start, uint32_t end) {
  if((start >= this->length || end > this->length) || (end <= start && end != 0)) {
    return 0;
  }

  if(end == 0) {
    return new String(&this->chars[start]);
  }

  char endChar = this->chars[end];
  this->chars[end] = 0;
  String* ret = new String(&this->chars[start]);
  this->chars[end] = endChar;
  return ret;
}

uint32_t String::strpos(char c, uint32_t occurrence) {
  uint32_t currentOccurrence = 0;
  for(int i = 0; i < this->length; i++) {
    if(this->chars[i] == c) {
      if(occurrence == currentOccurrence) {
        return i;
      } else {
        currentOccurrence++;
      }
    }
  }
  return this->length;
}

char* String::getCharPtr() {
  return this->chars;
}

int64_t String::parseInt() {
  int64_t ret = 0;
  bool minus = false;
  for(int i = 0; i < this->length; i++) {
    if(this->chars[i] >= ('0') && this->chars[i] < ('0') + 10) {
      ret = ret*10+(this->chars[i]-('0'));
    }
    if(this->chars[i] == '-') {
      minus = true;
    }
  }
  if(minus) ret *= -1;
  return ret;
}

bool String::contains(char* str, uint32_t strlen) {
  if(strlen == 0) {
    strlen = String::findLength(str);
  }

  uint32_t x = 0;
  for(int i = 0; i < this->length; i++) {
    if(this->chars[i] == str[x]) {
      x++;
    } else if(x != 0) {
      i -= (x-1);
      x == 0;
    }
    if(x >= strlen) {
      return true;
    }
  }
  return false;
}

bool String::equals(char* str) {
  uint32_t strlen = String::findLength(str);
  return (strlen == this->length && this->contains(str, strlen));
}

void String::append(char* str) {
  uint32_t strLen = String::findLength(str);
  uint32_t totalLen = this->length+strLen;
  char* tmp = (char *)MemoryManager::activeMemoryManager->malloc(sizeof(char) * (totalLen + 1));

  uint32_t current = 0;
  for(int i = 0; i < this->length; i++) {
    tmp[current] = this->chars[i];
    current++;
  }
  for(int i = 0; i < strLen; i++) {
    tmp[current] = str[i];
    current++;
  }
  tmp[current] = 0;

  delete this->chars;
  this->chars = tmp;
  this->length = totalLen;
}

uint32_t String::findLength(char* str) {
  char c = 0;
  uint32_t i = 0;
  do {
    c = str[i];
    i++;
  } while(c != 0);
  return i - 1; // Ignore 0.
}
