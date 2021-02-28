#include <audio/Audio.h>

using namespace sys::common;
using namespace sys::audio;

Audio* Audio::activeAudioManager = 0;

Audio::Audio() {
  Audio::activeAudioManager = this;
  this->driver = 0;
}

Audio::~Audio() {
  Audio::activeAudioManager = 0;
}

void Audio::registerDriver(drivers::audio::AudioDriver* driver) {
  this->driver = driver;
}

AudioStream* Audio::getStream() {
  return new AudioStream(this->driver);
}
