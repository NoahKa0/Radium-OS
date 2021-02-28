#include <audio/AudioStream.h>

using namespace sys::common;
using namespace sys::audio;

AudioStream::AudioStream(drivers::audio::AudioDriver* driver) {
  this->driver = driver;
  this->buffer = 0;
  if(this->driver != 0) {
    this->writePosition = 0;
    this->bufferSize = driver->preferedBufferSize() * sizeof(uint16_t);
    this->buffer = (uint16_t*) MemoryManager::activeMemoryManager->malloc(this->bufferSize);
  }
}

AudioStream::~AudioStream() {
  this->flush();
  if(this->buffer != 0) {
    MemoryManager::activeMemoryManager->free(this->buffer);
  }
}

void AudioStream::setVolume(uint8_t volume) {
  if(this->driver == 0) {
    return;
  }
  this->driver->setVolume(volume);
}

void AudioStream::write(uint8_t* samples, uint32_t sizeInBytes) {
  if(this->driver == 0) {
    return;
  }
  uint8_t* buffer = (uint8_t*) this->buffer;
  uint32_t bytesAvailable = 0;
  uint32_t bytesToWrite = 0;
  while(sizeInBytes > 0) {
    bytesAvailable = this->bufferSize - this->writePosition;
    if(bytesAvailable == 0) {
      this->flush();
      continue;
    }
    bytesToWrite = bytesAvailable > sizeInBytes ? sizeInBytes : bytesAvailable;

    for(uint32_t i = 0; i < bytesToWrite; i++) {
      buffer[this->writePosition + i] = samples[i];
    }
    this->writePosition += bytesToWrite;
    samples += bytesToWrite;
    sizeInBytes -= bytesToWrite;
  }
}

void AudioStream::flush() {
  if(this->driver == 0) {
    return;
  }
  uint16_t* samples = this->buffer;
  uint32_t sizeInBytes = this->writePosition;

  uint32_t sizeInSamples = sizeInBytes / 2;
  if(this->isBigEndian != this->driver->isBigEndian()) {
    for(uint32_t i = 0; i < sizeInSamples; i++) {
      samples[i] = ((samples[i] << 8) & 0xFF00) | ((samples[i] >> 8) & 0x00FF);
    }
  }
  uint8_t* origin = (uint8_t*) samples;
  uint32_t bytesReady = 0;
  uint32_t sleep = 0;
  while(sizeInBytes > 0) {
    sleep = 0;
    while(this->driver->samplesReady() == 0) {
      SystemTimer::sleep(80);
      sleep++;
      if(sleep > 100) {
        this->driver->stop();
        this->writePosition = 0;
        return;
      }
    }
    bytesReady = this->driver->samplesReady() * this->driver->getChannels() * sizeof(uint16_t);
    if(bytesReady > sizeInBytes) {
      bytesReady = sizeInBytes;
    }
    this->driver->write(origin, bytesReady);
    origin += bytesReady;
    sizeInBytes -= bytesReady;
  }
  this->writePosition = 0;
}

void AudioStream::stop() {
  this->writePosition = 0;
  if(this->driver != 0) {
    this->driver->stop();
  }
}

void AudioStream::setBigEndian(bool isBE) {
  this->isBigEndian = isBE;
}
