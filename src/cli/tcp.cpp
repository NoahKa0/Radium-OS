#include <cli/tcp.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printHex32(uint32_t);
void printHex8(uint8_t);
TransmissionControlProtocolProvider* getTCP();

CmdTCP::CmdTCP() {}
CmdTCP::~CmdTCP() {}

void CmdTCP::run(common::String** args, common::uint32_t argsLength) {
  TransmissionControlProtocolProvider* tcp = getTCP();
  bool connectionFailed = false;
  if(tcp == 0) {
    printf("No connection!");
    return;
  }
  if(argsLength < 2) {
    printf("Usage: <ip> <port>\n");
    return;
  }
  if(args[0]->occurrences('.') != 3) {
    printf("IP must be like x.x.x.x");
    return;
  }
  String* ip1 = args[0]->split('.', 0);
  String* ip2 = args[0]->split('.', 1);
  String* ip3 = args[0]->split('.', 2);
  String* ip4 = args[0]->split('.', 3);
  uint32_t ip_BE = ((ip1->parseInt() & 0xFF))
              + ((ip2->parseInt() & 0xFF) << 8)
              + ((ip3->parseInt() & 0xFF) << 16)
              + ((ip4->parseInt() & 0xFF) << 24);
  uint32_t port = args[1]->parseInt();
  printf("Connecting to: ");
  printHex8((uint8_t) ip1->parseInt());
  printf(".");
  printHex8((uint8_t) ip2->parseInt());
  printf(".");
  printHex8((uint8_t) ip3->parseInt());
  printf(".");
  printHex8((uint8_t) ip4->parseInt());
  printf(" on port 0x");
  printHex32(port);
  printf("... ");

  this->socket = tcp->connect(ip_BE, port);

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

  delete ip1;
  delete ip2;
  delete ip3;
  delete ip4;
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
