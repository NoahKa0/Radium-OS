#include <test/cli.h>
#include <memorymanagement.h>
#include <timer.h>
#include <drivers/vga.h>

using namespace sys;
using namespace sys::test;
using namespace sys::common;
using namespace sys::net;

void printf(char*);
void printHex32(uint32_t);
void printHex8(uint8_t);
InternetControlMessageProtocol* getICMP();
TransmissionControlProtocolProvider* getTCP();

Cmd::Cmd() {}
Cmd::~Cmd() {}
void Cmd::run(common::String** args, common::uint32_t argsLength) {}
void Cmd::onInput(common::String* input) {}


Cli::Cli() : KeyboardEventHandler() {
  this->cmdBufferSize = 512;
  this->cmdBufferPos = 0;
  this->cmdBuffer = (char*) MemoryManager::activeMemoryManager->malloc(sizeof(char) * (this->cmdBufferSize + 1));
  this->cmd = 0;
  this->command = 0;
  this->running = true;
}

Cli::~Cli() {
  delete this->cmdBuffer;
}

void Cli::onKeyDown(char c) {
  char* charToPrint = " ";
  if(c == '\n') { // New line
    this->cmdBuffer[this->cmdBufferPos] = 0;
    if(this->command != 0 && this->cmd == 0) {
      return;
    }
    if(this->cmd != 0) {
      String* tmp = new String(this->cmdBuffer);
      this->cmd->onInput(tmp);
      delete tmp;
    } else {
      this->command = new String(this->cmdBuffer);
    }
    this->cmdBufferPos = 0;
  } else if(c == '\b') { // Backspace
    if(this->cmdBufferPos != 0) {
      charToPrint[0] = '\b';
      printf(charToPrint);
      this->cmdBufferPos--;
    }
  } else { // Normal char
    if(this->cmdBufferPos < this->cmdBufferSize) {
      charToPrint[0] = c;
      printf(charToPrint);
      this->cmdBuffer[this->cmdBufferPos] = c;
      this->cmdBufferPos++;
    } else {
      printf("Too many chars!\n");
      printf("> ");
      this->cmdBufferPos = 0;
    }
  }
}

void Cli::run() {
  printf(">");
  while(this->running) {
    if(this->command != 0) {
      printf("\n");
      String* read = this->command;
      String* command = read->split(' ', 0);
      Cmd* cmd = 0;

      if(command->equals("tcp")) {
        cmd = new CmdTCP();
      } else if(command->equals("ping")) {
        cmd = new CmdPing();
      } else if(command->equals("img")) {
        cmd = new CmdIMG();
      } else { // Only show text
        if(command->equals("help")) {
          printf("--- HELP ---\n");
          printf("help: Shows this\n");
          printf("tcp <ip> <port>: Send and receive text over TCP\n");
          printf("ping <ip> <times>: Pings an ip\n");
        } else {
          printf("Invalid command!\n");
        }
        printf(">");
        this->command = 0;
        delete command;
        delete read;
        continue;
      }

      this->cmd = cmd;

      uint32_t argsCount = read->occurrences(' ');
      String** args = (String**) MemoryManager::activeMemoryManager->malloc(sizeof(String*) * argsCount);
      for(int i = 0; i < argsCount; i++) {
        args[i] = read->split(' ', i + 1);
      }
      printf("\n");
      cmd->run(args, argsCount);

      this->cmd = 0;
      this->command = 0;

      for(int i = 0; i < argsCount; i++) {
        delete args[i];
      }
      delete args;
      delete command;
      delete read;
      delete cmd;
      printf("\n>");
    }
    asm("hlt");
  }
}

void enableVGA();
drivers::VideoGraphicsArray* getVGA();

CmdPing::CmdPing() : Cmd() {}
CmdPing::~CmdPing() {}

void CmdPing::run(String** args, uint32_t argsLength) {
  InternetControlMessageProtocol* icmp = getICMP();
  if(icmp == 0) {
    printf("No connection!");
    return;
  }
  if(argsLength < 2) {
    printf("Usage: <ip> <times>\n");
    return;
  }
  if(args[0]->occurrences('.') != 3) {
    printf("Invalid IP! IP must be like x.x.x.x");
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
  uint32_t times = args[1]->parseInt();
  printf("Pinging ");
  printHex32(ip_BE);
  printf(" 0x");
  printHex32(times);
  printf(" times\n");

  for(int i = 0; i < times; i++) {
    printf("PINGING: ");
    printHex8((uint8_t) ip1->parseInt());
    printf(".");
    printHex8((uint8_t) ip2->parseInt());
    printf(".");
    printHex8((uint8_t) ip3->parseInt());
    printf(".");
    printHex8((uint8_t) ip4->parseInt());
    printf("\n");

    icmp->ping(ip_BE);

    SystemTimer::sleep(500);
  }

  delete ip1;
  delete ip2;
  delete ip3;
  delete ip4;
}


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
    printf("Invalid IP! IP must be like x.x.x.x");
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
  printf("\n");

  this->socket = tcp->connect(ip_BE, port);

  for(int i = 0; i < 10 && !this->socket->isConnected(); i++) {
    SystemTimer::sleep(1000);
  }

  if(!this->socket->isConnected()) {
    printf("TCP: Connection failed!\n");
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
      printf("\nTCP RECV: ");
      printf((char*) data);
      printf("\n");
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
      if(i == 9) {
        printf("TCP: Ungraceful disconnect!\n");
      }
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
    printf("TCP: Not connected!\n");
  }
}


CmdIMG::CmdIMG() : Cmd() {}
CmdIMG::~CmdIMG() {}

void CmdIMG::run(common::String** args, common::uint32_t argsLength) {
  TransmissionControlProtocolProvider* tcp = getTCP();
  TransmissionControlProtocolSocket* socket = 0;
  bool connectionFailed = false;
  if(tcp == 0) {
    printf("No connection!");
    return;
  }
  if(argsLength < 1) {
    printf("Usage: <ip>\n");
    return;
  }
  if(args[0]->occurrences('.') != 3) {
    printf("Invalid IP! IP must be like x.x.x.x");
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
  uint32_t port = 1234;
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
  printf("\n");

  socket = tcp->connect(ip_BE, port);

  for(int i = 0; i < 10 && !socket->isConnected(); i++) {
    SystemTimer::sleep(1000);
  }

  if(!socket->isConnected()) {
    printf("TCP: Connection failed!\n");
    connectionFailed = true;
  } else {
    printf("Connected!\n");
  }

  uint32_t command = 0;
  char* msg = "Got it";
  uint32_t recvd = 0;

  if(socket->isConnected()) {
    enableVGA();
  }

  while(socket->isConnected()) {
    if(socket->hasNext() != 0) {
      uint32_t bytesToRead = socket->hasNext();
      uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(bytesToRead);
      socket->readNext(data, bytesToRead);
      for(int i = 0; i < bytesToRead; i++) {
        if(data[i] == 175 && command > 4) {
          command = 0;
          continue;
        }
        switch(command) {
          default:
            break;
          case 0:
            this->r = data[i];
            break;
          case 1:
            this->g = data[i];
            break;
          case 2:
            this->b = data[i];
            break;
          case 3:
            this->x = data[i];
            break;
          case 4:
            this->y = data[i];
            if(this->x < 320 && this->y < 200) {
              getVGA()->putPixel(this->x, this->y, this->r, this->g, this->b);
              recvd++;
            }
            break;
        }
        command = command+1;
        if(recvd > 25) {
          recvd = 0;
          socket->send((uint8_t*) msg, 6);
        }
      }
    } else {
      socket->sendExpiredPackets();
    }
    asm("hlt");
  }

  printf("TCP: Disconnect!\n");

  if(!connectionFailed) {
    for(int i = 0; i < 10 && !socket->isClosed(); i++) {
      SystemTimer::sleep(1000);
      if(i == 9) {
        printf("TCP: Ungraceful disconnect!\n");
      }
    }
    while(true) {
      asm("hlt");
    }
  }

  delete ip1;
  delete ip2;
  delete ip3;
  delete ip4;
  delete socket;
  socket = 0;
}