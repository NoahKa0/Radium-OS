#include <cli/audio.h>
#include <drivers/storage/storageDevice.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::audio;

void printf(const char*);
void printHex32(uint32_t);
void printHex8(uint8_t);


CmdAUDIO::CmdAUDIO() {
  this->shouldStop = false;
}
CmdAUDIO::~CmdAUDIO() {}

void CmdAUDIO::run(String** args, uint32_t argsLength) {
  if(this->workingDirectory == 0) {
    printf("Working directory not set!\n");
    return;
  }

  String* filename = new String();
  for(int i = 0; i < argsLength; i++) {
    filename->append(args[i]->getCharPtr());
    if(i+1 != argsLength) {
      filename->append(" ");
    }
  }

  File* tmp = this->workingDirectory->getChildByName(filename);
  if(tmp != 0) {
    printf("Starting to play audio!\nPress enter to stop...\n");
    AudioStream* stream = Audio::activeAudioManager->getStream();
    stream->setVolume(255);
    stream->setBigEndian(false);

    uint32_t length = tmp->hasNext();
    uint32_t readAtOnce = 512;
    uint8_t* content = (uint8_t*) MemoryManager::activeMemoryManager->malloc(readAtOnce + 1);
    uint32_t read = 0;
    while(read < length && !this->shouldStop) {
      uint32_t toRead = length - read;
      if(toRead > readAtOnce) toRead = readAtOnce;
      tmp->read(content, toRead);
      read += toRead;
      stream->write(content, toRead);
    }

    if(this->shouldStop) {
      stream->stop(); // Stop forces the driver to stop playing instead of finishing the audio still in the buffer.
    }

    delete content;
    delete stream;
    delete tmp;
  }
  delete filename;
}

void CmdAUDIO::onInput(common::String* input) {
  this->shouldStop = true;
}
