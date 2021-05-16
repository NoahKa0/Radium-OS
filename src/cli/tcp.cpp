#include <cli/tcp.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printNum(uint32_t);

CmdTCP::CmdTCP() {}
CmdTCP::~CmdTCP() {}

void CmdTCP::run(common::String** args, common::uint32_t argsLength) {
  bool connectionFailed = false;
  if(argsLength < 2) {
    printf("Usage: <ip> <port>\n");
    return;
  }
  String* ip = NetworkManager::networkManager->getIpString(args[0]);
  uint32_t port = args[1]->parseInt();
  printf("Connecting to: ");
  printf(ip->getCharPtr());
  printf(" on port ");
  printNum(port);
  printf("... ");

  this->socket = NetworkManager::networkManager->connectTCP(ip, port);
  if (this->socket == 0) {
    printf("\nCould not resolve: ");
    printf(args[0]->getCharPtr());
    printf("\n");
    delete ip;
    return;
  }

  for(int i = 0; i < 10 && !this->socket->isConnected(); i++) {
    SystemTimer::sleep(1000);
  }

  if(!this->socket->isConnected()) {
    printf("failed!\n");
    connectionFailed = true;
  } else {
    printf("Connected!\n");
  }

  while(this->socket->isConnected()) {
    if(this->socket->hasNext() != 0) {
      uint32_t bytesToRead = this->socket->hasNext();
      uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(bytesToRead + 1);
      this->socket->readNext(data, bytesToRead);
      data[bytesToRead] = 0;
      printf((char*) data);
      delete data;
    } else {
      this->socket->sendExpiredPackets();
    }
    asm("hlt");
  }

  printf("TCP: Disconnect!\n");

  if(!connectionFailed) {
    for(int i = 0; i < 10 && !this->socket->isClosed(); i++) {
      SystemTimer::sleep(1000);
    }
  }

  delete ip;
  delete this->socket;
  this->socket = 0;
}

void CmdTCP::onInput(common::String* input) {
  printf("\n");
  if(this->socket != 0 && !this->socket->isClosed() && this->socket->isConnected()) {
    input->append("\n");
    this->socket->send((uint8_t*) input->getCharPtr(), input->getLength());
  } else {
    printf("Not connected!\n");
  }
}
