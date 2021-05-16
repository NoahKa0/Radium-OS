#include <cli/ping.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printHex32(uint32_t);
void printHex8(uint8_t);

CmdPing::CmdPing() : Cmd() {}
CmdPing::~CmdPing() {}

void CmdPing::run(String** args, uint32_t argsLength) {
  if(argsLength < 2) {
    printf("Usage: <ip> <times>\n");
    return;
  }
  String* ip = NetworkManager::networkManager->getIpString(args[0]);
  uint32_t times = args[1]->parseInt();
  if (times > 30) {
    times = 30;
  }
  printf("Pinging ");
  printf(ip->getCharPtr());
  printf(" 0x");
  printHex32(times);
  printf(" times\n");

  for(int i = 0; i < times; i++) {
    printf("PINGING: ");
    printf(ip->getCharPtr());
    printf("\n");

    NetworkManager::networkManager->ping(ip);

    SystemTimer::sleep(500);
  }

  delete ip;
}
