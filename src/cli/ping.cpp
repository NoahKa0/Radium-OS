#include <cli/ping.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printNum(uint32_t);
void printIp(uint32_t ip, bool bigEndian);

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
  uint32_t dstIp_BE = NetworkManager::networkManager->parseIp(ip);
  printf("Pinging ");
  printIp(dstIp_BE, true);
  printf(" ");
  printNum(times);
  printf(" times\n");

  for(int i = 0; i < times; i++) {
    printf("PINGING: ");
    printIp(dstIp_BE, true);
    printf(": ");

    uint32_t responseIp = NetworkManager::networkManager->ping(ip, 0x40);
    if (responseIp == 0) {
      printf("*\n");
    } else {
      printf("RESPONSE FROM: ");
      printIp(responseIp, true);
      printf("\n");
    }

    SystemTimer::sleep(250);
  }

  delete ip;
}
