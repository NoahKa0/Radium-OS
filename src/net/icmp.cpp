#include <net/icmp.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;

InternetControlMessageProtocol::InternetControlMessageProtocol(InternetProtocolV4Provider* backend)
:InternetProtocolV4Handler(backend, 0x01) {}

InternetControlMessageProtocol::~InternetControlMessageProtocol() {}

void printf(char* txt);
void printHex8(uint8_t num);
void printHex32(uint32_t num);

bool InternetControlMessageProtocol::onInternetProtocolReceived(uint32_t srcIp_BE, uint32_t destIp_BE, uint8_t* payload, uint32_t size) {
  if(size < sizeof(InternetControlMessageProtocolHeader)) return false;
  
  InternetControlMessageProtocolHeader* msg = (InternetControlMessageProtocolHeader*) payload;
  
  printf("PING FROM: ");
  printHex8((uint8_t)((srcIp_BE) & 0xFF));
  printf(".");
  printHex8((uint8_t)((srcIp_BE >> 8) & 0xFF));
  printf(".");
  printHex8((uint8_t)((srcIp_BE >> 16) & 0xFF));
  printf(".");
  printHex8((uint8_t)((srcIp_BE >> 24) & 0xFF));
  printf(" DATA: ");
  printHex32(msg->data);
  
  switch(msg->type) {
    case 0:
      printf(" RESPONSE");
      break;
    case 8:
      printf(" REQUEST\n");
      msg->type = 0;
      msg->checksum = 0;
      msg->checksum = InternetProtocolV4Provider::checksum((uint16_t*) &msg, sizeof(InternetControlMessageProtocolHeader));
      return true;
    default:
      break;
  }
  printf("\n");
  
  return false;
}

void InternetControlMessageProtocol::ping(uint32_t ip_be) {
  InternetControlMessageProtocolHeader icmp;
  icmp.type = 8; // 8 = ping.
  icmp.code = 0;
  icmp.data = 3713; // BE 1337 = "LEET"
  icmp.checksum = 0;
  icmp.checksum = InternetProtocolV4Provider::checksum((uint16_t*) &icmp, sizeof(InternetControlMessageProtocolHeader));
  
  this->send(ip_be, (uint8_t*) &icmp, sizeof(InternetControlMessageProtocolHeader));
}
