#include <drivers/audio/AudioDriver.h>

using namespace sys;
using namespace sys::common;
using namespace sys::hardware;
using namespace sys::drivers;
using namespace sys::drivers::audio;

AudioDriver::AudioDriver()
:Driver()
{}

AudioDriver::~AudioDriver() {}

void AudioDriver::setVolume(common::uint8_t volume) {}

bool AudioDriver::isBigEndian() {
  return false;
}

uint8_t AudioDriver::getChannels() {
  return 1;
}

uint8_t AudioDriver::getSampleSize() {
  return 1;
}

void AudioDriver::write(common::uint8_t* samples, common::uint32_t sizeInBytes) {

}

uint32_t AudioDriver::samplesReady() {
  return 0;
}
