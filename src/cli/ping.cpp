#include <cli/ping.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;

void printf(const char*);
void printHex32(uint32_t);
void printHex8(uint8_t);
InternetControlMessageProtocol* getICMP();

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
