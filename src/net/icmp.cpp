#include <net/icmp.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;

InternetControlMessageProtocol::InternetControlMessageProtocol(InternetProtocolV4Provider* backend)
:InternetProtocolV4Handler(backend, 0x01)
{
  this->hasResponse = false;
  this->isWaiting = false;
  this->responseIp = 0;
}

InternetControlMessageProtocol::~InternetControlMessageProtocol() {}

void printf(const char* txt);
void printNum(uint32_t num);
void printHex32(uint32_t num);
void printIp(uint32_t ip, bool bigEndian);

bool InternetControlMessageProtocol::onInternetProtocolReceived(uint32_t srcIp_BE, uint32_t destIp_BE, uint8_t* payload, uint32_t size) {
  if(size < sizeof(InternetControlMessageProtocolHeader)) return false;
  
  InternetControlMessageProtocolHeader* msg = (InternetControlMessageProtocolHeader*) payload;
  
  switch(msg->type) {
    // Normal responses
    case 0:
      if (this->isWaiting && !this->hasResponse) {
        this->hasResponse = true;
        this->responseIp = srcIp_BE;
      } else {
        printf("UNKNOWN ICMP RESPONSE FROM: ");
        printIp(srcIp_BE, true);
        printf(" DATA: ");
        printHex32(msg->data);
        printf("\n");
      }
      break;
    // Abnormal responses
    case 11: // TTL exceeded
      if (this->isWaiting && !this->hasResponse) {
        this->hasResponse = true;
        this->responseIp = srcIp_BE;
      }
      break;
    // Requests
    case 8:
      printf("ICMP REQUEST FROM: ");
      printIp(srcIp_BE, true);
      printf(" DATA: ");
      printHex32(msg->data);
      printf("\n");
      msg->type = 0;
      msg->checksum = 0;
      msg->checksum = InternetProtocolV4Provider::checksum((uint16_t*) msg, size);
      return true;
    default:
      break;
  }
  return false;
}

uint32_t InternetControlMessageProtocol::ping(uint32_t ip_be, uint8_t ttl) {
  if (this->isWaiting) {
    return 0;
  }
  InternetControlMessageProtocolHeader icmp;
  icmp.type = 8; // 8 = ping.
  icmp.code = 0;
  icmp.data = 3713; // BE 1337 = "LEET"
  icmp.checksum = 0;
  icmp.checksum = InternetProtocolV4Provider::checksum((uint16_t*) &icmp, sizeof(InternetControlMessageProtocolHeader));

  this->hasResponse = false;
  this->isWaiting = true;

  this->send(ip_be, (uint8_t*) &icmp, sizeof(InternetControlMessageProtocolHeader), ttl);

  uint32_t ret = 0;
  for (uint32_t i = 0; i < 20; i++) {
    if (this->hasResponse) {
      ret = this->responseIp;
      break;
    }
    SystemTimer::sleep(150);
  }
  this->isWaiting = false;
  this->hasResponse = false;
  return ret;
}
