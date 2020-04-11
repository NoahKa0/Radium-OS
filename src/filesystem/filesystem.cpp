#include <filesystem/filesystem.h>

using namespace sys::common;
using namespace sys::filesystem;

File::File(FileSystem* fileSystem) {
  this->fileSystem = fileSystem;
}

File::~File() {}

bool File::isFolder() {
  return false;
}

File* File::getParent() {
  return 0;
}

File* File::getChild(uint32_t file) {
  return 0;
}

File* File::getChildByName(String* file) {
  return 0;
}

uint32_t File::numChildren() {
  return 0;
}

bool File::mkFile(String* filename, bool folder) {
  return false;
}

bool File::append(uint8_t* data, uint32_t length) {
  return false;
}

int File::hasNext() {
  return 0;
}

uint8_t File::nextByte() {
  return 0;
}

void File::read(uint8_t* buffer, uint32_t length) {}

String* File::getFilename() {
  return new String();
}


FileSystem* File::getFileSystem() {
  return this->fileSystem;
}

FileSystem::FileSystem() {}
FileSystem::~FileSystem() {}

File* FileSystem::getRoot() {
  return 0;
}

String* FileSystem::getName() {
  return 0;
}

String* FileSystem::getType() {
  return 0;
}
