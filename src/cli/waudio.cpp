#include <cli/waudio.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;
using namespace sys::net::tcp;
using namespace sys::audio;

void printf(const char*);
void printNum(uint32_t);

CmdWAUDIO::CmdWAUDIO() {
  this->shouldStop = false;
}
CmdWAUDIO::~CmdWAUDIO() {}

void CmdWAUDIO::run(String** args, uint32_t argsLength) {
  bool connectionFailed = false;
  uint32_t responseCode = 0;
  uint32_t contentLength = 0;
  uint32_t bytesReceived = 0;

  if(argsLength < 2) {
    printf("Usage: <host> <query>\n");
    return;
  }

  String* ip = NetworkManager::networkManager->getIpString(args[0]);
  String* query = new String(args[1]->getCharPtr());
  printf("Connecting to: ");
  printf(ip->getCharPtr());
  printf("... ");

  TransmissionControlProtocolSocket* socket = NetworkManager::networkManager->connectTCP(ip, 80);
  if (socket == 0) {
    printf("\nCould not resolve: ");
    printf(args[0]->getCharPtr());
    printf("\n");
    delete query;
    delete ip;
    return;
  }

  for(int i = 0; i < 10 && !socket->isConnected(); i++) {
    SystemTimer::sleep(1000);
  }

  if(socket->isConnected()) {
    printf("Connected!\n");
    Http* httpClient = new Http(socket);
    httpClient->request(args[0], query);
    responseCode = httpClient->getResponseCode();
    contentLength = httpClient->getContentLength();
    delete httpClient;
  } else {
    printf("failed!\n");
    connectionFailed = true;
  }

  if (contentLength < 1) {
    contentLength = 1;
  }

  uint64_t waitUntil = 0;
  uint32_t readAtOnce = 0xFF0C;
  uint32_t secondsPassed = 1;

  AudioStream* stream = Audio::activeAudioManager->getStream();
  stream->setVolume(255);
  stream->setBigEndian(false);

  uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(readAtOnce + 1);

  if (responseCode == 200) {
    printf("Buffering...");
    SystemTimer::sleep(5000);
    printf("\n");
    while((socket->isConnected() || socket->hasNext() != 0) && !this->shouldStop && bytesReceived < contentLength) {
      if(socket->hasNext() != 0) {
        uint32_t bytesToRead;
        do {
          bytesToRead = socket->hasNext();
          SystemTimer::sleep(150);
        } while(bytesToRead < socket->hasNext() && bytesToRead < readAtOnce);
        if (bytesToRead > readAtOnce) {
          bytesToRead = readAtOnce;
        }
        socket->readNext(data, bytesToRead);
        stream->write(data, bytesToRead);

        bytesReceived += bytesToRead;
        if (SystemTimer::getTimeInInterrupts() > waitUntil) {
          uint32_t secondsLeft = (contentLength - bytesReceived) * secondsPassed / bytesReceived;
          printf("Received ");
          printNum(bytesReceived * 100 / contentLength);
          printf("%, ");
          printNum(secondsLeft / 60);
          printf(" minutes and ");
          printNum(secondsLeft % 60);
          printf(" seconds left\n");
          waitUntil = SystemTimer::getTimeInInterrupts() + SystemTimer::millisecondsToLength(1000);
          secondsPassed++;
        }
      } else {
        socket->sendExpiredPackets();
      }
      asm("hlt");
    }
  } else {
    printf("HTTP failed: ");
    printNum(responseCode);
    printf("\n");
  }
  if (this->shouldStop) {
    printf("Cancelled by user!\n");
    stream->stop();
  }

  delete data;
  delete stream;

  if(!connectionFailed) {
    for(int i = 0; i < 10 && !socket->isClosed(); i++) {
      SystemTimer::sleep(1000);
    }
  }

  delete query;
  delete ip;
  delete socket;
}

void CmdWAUDIO::onInput(common::String* input) {
  this->shouldStop = true;
}
