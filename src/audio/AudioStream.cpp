#include <audio/AudioStream.h>

using namespace sys::common;
using namespace sys::audio;

void printf(const char* str);

AudioStream::AudioStream(drivers::audio::AudioDriver* driver) {
  this->driver = driver;
}

AudioStream::~AudioStream() {

}

void AudioStream::setVolume(uint8_t volume) {
  if(this->driver == 0) {
    return;
  }
  this->driver->setVolume(volume);
}

void AudioStream::write(uint16_t* samples, uint32_t sizeInBytes) {
  if(this->driver == 0) {
    return;
  }
  uint32_t sizeInSamples = sizeInBytes / 2;
  if(this->isBigEndian != this->driver->isBigEndian()) {
    printf("Swap endian!\n");
    for(uint32_t i = 0; i < sizeInSamples; i++) {
      samples[i] = ((samples[i] << 8) & 0xFF00) | ((samples[i] >> 8) & 0x00FF);
    }
  }
  uint8_t* origin = (uint8_t*) samples;
  uint32_t bytesReady = 0;
  while(sizeInBytes > 0) {
    while(this->driver->samplesReady() == 0) {
      SystemTimer::sleep(40);
    }
    uint32_t bytesReady = this->driver->samplesReady() * 2 * 2;
    if(bytesReady > sizeInBytes) {
      bytesReady = sizeInBytes;
    }
    this->driver->write(origin, bytesReady);
    origin += bytesReady;
    sizeInBytes -= bytesReady;
  }
}

void AudioStream::setBigEndian(bool isBE) {
  this->isBigEndian = isBE;
}
