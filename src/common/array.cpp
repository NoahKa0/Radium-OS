#include <common/array.h>
#include <memorymanagement/memorymanagement.h>

using namespace sys;
using namespace sys::common;

Array::Array(bool autoclean) {
  this->autoclean = autoclean;
  this->length = 0;
  this->bufferLength = 32;
  this->items = (uint8_t**) MemoryManager::activeMemoryManager->malloc(sizeof(uint8_t*) * this->bufferLength);
  for(int i = 0; i < this->bufferLength; i++) {
    this->items[i] = 0;
  }
}

Array::~Array() {
  if(this->autoclean) {
    for(int i = 0; i < this->length; i++) {
      if(this->items[i] != 0) {
        delete this->items[i];
        this->items[i] = 0;
      }
    }
  }
  delete this->items;
}

uint32_t Array::getLength() {
  return this->length;
}

uint8_t* Array::get(uint32_t n) {
  if(n >= this->length) {
    return 0;
  }
  return this->items[n];
}

void Array::set(uint32_t n, uint8_t* data) {
  if(n >= this->length) {
    if(this->autoclean) {
      delete data;
    }
    return;
  }
  if(this->autoclean && this->items[n] != 0) {
    delete this->items[n];
  }
  this->items[n] = data;
}

void Array::add(uint8_t* data) {
  if(this->length >= this->bufferLength - 1) {
    uint8_t** tmp = (uint8_t**) MemoryManager::activeMemoryManager->malloc(sizeof(uint8_t*) * this->bufferLength * 2);
    for(int i = 0; i < this->bufferLength; i++) {
      tmp[i] = this->items[i];
    }
    delete this->items;
    this->items = tmp;
    this->bufferLength = this->bufferLength * 2;
  }
  this->items[this->length] = data;
  this->length++;
}

void Array::remove(uint32_t n) {
  if(n >= this->length) {
    return;
  }
  if(this->autoclean) {
    delete this->items[n];
  }
  for(int i = n; i < this->length; i++) {
    this->items[i] = this->items[i + 1];
  }
  this->length--;
}
