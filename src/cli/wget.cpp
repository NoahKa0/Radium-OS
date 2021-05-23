#include <cli/wget.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printNum(uint32_t);

CmdWGET::CmdWGET() {
  this->appendFile = 0;
  this->enteredFile = false;
  this->pressedEnter = 0;
}
CmdWGET::~CmdWGET() {
  if (this->appendFile != 0) {
    delete this->appendFile;
  }
}

void CmdWGET::run(common::String** args, common::uint32_t argsLength) {
  bool connectionFailed = false;
  uint32_t responseCode = 0;
  uint32_t contentLength = 0;
  uint32_t bytesReceived = 0;

  if(this->workingDirectory == 0) {
    printf("No working directory!\n");
    return;
  }
  if(argsLength < 2) {
    printf("Usage: <host> <query>\n");
    return;
  }

  printf("Enter filename: ");

  while (this->appendFile == 0 && !this->enteredFile) {
    asm("hlt");
  }

  if (this->appendFile == 0) {
    return;
  }

  String* ip = NetworkManager::networkManager->getIpString(args[0]);
  String* query = new String(args[1]->getCharPtr());
  printf("Connecting to: ");
  printf(ip->getCharPtr());
  printf("... ");

  this->socket = NetworkManager::networkManager->connectTCP(ip, 80);
  if (this->socket == 0) {
    printf("\nCould not resolve: ");
    printf(args[0]->getCharPtr());
    printf("\n");
    delete query;
    delete ip;
    return;
  }

  for(int i = 0; i < 10 && !this->socket->isConnected(); i++) {
    SystemTimer::sleep(1000);
  }

  if(this->socket->isConnected()) {
    printf("Connected!\n");
    Http* httpClient = new Http(this->socket);
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
  uint32_t readAtOnce = 0xFFFF;
  uint32_t secondsPassed = 1;

  uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(readAtOnce + 1);

  if (responseCode == 200) {
    while((this->socket->isConnected() || this->socket->hasNext() != 0) && this->pressedEnter < 6 && bytesReceived < contentLength) {
      if(this->socket->hasNext() != 0) {
        uint32_t bytesToRead = this->socket->hasNext();
        if (bytesToRead > readAtOnce) {
          bytesToRead = readAtOnce;
        }
        this->socket->readNext(data, bytesToRead);
        this->appendFile->append(data, bytesToRead);
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
        this->socket->sendExpiredPackets();
      }
      asm("hlt");
    }
    this->workingDirectory->reset();
  } else {
    printf("HTTP failed: ");
    printNum(responseCode);
    printf("\n");
  }
  delete data;

  if (bytesReceived < contentLength && responseCode == 200 && !this->socket->isConnected()) {
    printf("Download cancelled by user!\n");
  }

  if(!connectionFailed) {
    for(int i = 0; i < 10 && !this->socket->isClosed(); i++) {
      SystemTimer::sleep(1000);
    }
  }

  delete query;
  delete ip;
  delete this->socket;
  this->socket = 0;
}

void CmdWGET::onInput(common::String* input) {
  if (this->appendFile == 0 && this->workingDirectory != 0) {
    bool result = this->workingDirectory->mkFile(input, false);
    if(!result) {
      printf("\nFile already exists!");
      this->enteredFile = true;
      return;
    }
    this->appendFile = this->workingDirectory->getChildByName(input);
  }
  this->enteredFile = true;
  this->pressedEnter++;
  printf("\n");
}
