#include <cli/traceroute.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printNum(uint32_t);
void printIp(uint32_t ip, bool bigEndian);

CmdTraceRoute::CmdTraceRoute() : Cmd() {}
CmdTraceRoute::~CmdTraceRoute() {}

void CmdTraceRoute::run(String** args, uint32_t argsLength) {
  if(argsLength < 1) {
    printf("Usage: <ip>\n");
    return;
  }
  String* ip = NetworkManager::networkManager->getIpString(args[0]);
  uint32_t dstIp_BE = NetworkManager::networkManager->parseIp(ip);

  uint8_t maxTtl = 0x40;
  printf("Tracing route to ");
  printIp(dstIp_BE, true);
  printf(" with max TTL ");
  printNum(maxTtl);
  printf("\n");

  for(uint8_t ttl = 1; ttl <= maxTtl; ttl++) {
    printf("PINGING: ");
    printIp(dstIp_BE, true);
    printf(": ");

    uint32_t amountOfTries = 0;
    uint32_t responseIp;
    do {
      responseIp = NetworkManager::networkManager->ping(ip, ttl);
      amountOfTries++;
    } while(responseIp == 0 && amountOfTries < 3);

    printf("TTL ");
    printNum(ttl);
    printf(" TRIES ");
    printNum(amountOfTries);
    printf(" = ");
    if (responseIp == 0) {
      printf("*\n");
    } else {
      printIp(responseIp, true);
      printf("\n");
    }

    if (responseIp == dstIp_BE) {
      printf("Trace complete with ");
      printNum(ttl);
      printf(" hops!\n");
      break;
    }

    SystemTimer::sleep(250);
  }

  delete ip;
}
