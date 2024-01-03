#include <net/NetworkManager.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::net::tcp;
using namespace sys::net::udp;

NetworkManager* NetworkManager::networkManager = 0;

NetworkManager::NetworkManager() {
  this->etherframe = 0;
  this->arp = 0;
  this->ipv4 = 0;
  this->icmp = 0;
  this->udp = 0;
  this->dhcp = 0;
  this->tcp = 0;

  this->currentDriver = 0;
  this->fallbackDriver = new drivers::EthernetDriver();

  if (this->networkManager == 0) {
    this->networkManager = this;
  }
}

NetworkManager::~NetworkManager() { // Will never be called.
  this->networkManager = 0;
}

void NetworkManager::registerEthernetDriver(drivers::EthernetDriver* drv) {
  if (!this->foundDriver()) {
    this->currentDriver = drv;
  }
}

void NetworkManager::setup() {
  if (this->etherframe != 0) { // Setup was already called.
    return;
  }
  drivers::EthernetDriver* driver = this->currentDriver;
  if (driver == 0) {
    driver = this->fallbackDriver;
  }

  this->etherframe = new EtherFrameProvider(driver);
  this->arp = new AddressResolutionProtocol(etherframe);
  this->ipv4 = new InternetProtocolV4Provider(etherframe, arp); // 0x00FFFFFF = 255.255.255.0 (subnet mask)
  this->icmp = new InternetControlMessageProtocol(ipv4);
  this->udp = new UserDatagramProtocolProvider(ipv4);
  this->tcp = new TransmissionControlProtocolProvider(ipv4);
  this->dhcp = new DynamicHostConfigurationProtocol(udp, ipv4);
  this->dns = new DomainNameSystem(tcp);

  this->hasLink();
}

uint32_t NetworkManager::ping(String* dest, uint8_t ttl) {
  this->hasLink();
  uint32_t ip = this->parseIp(dest);
  if (this->icmp == 0 || ip == 0) {
    return 0;
  }
  return this->icmp->ping(ip, ttl);
}

TransmissionControlProtocolSocket* NetworkManager::connectTCP(String* dest, uint16_t port, uint16_t localPort) {
  this->hasLink();
  uint32_t ip = this->parseIp(dest);
  if (this->tcp == 0 || ip == 0) {
    return 0;
  }
  return this->tcp->connect(ip, port, localPort);
}

UserDatagramProtocolSocket* NetworkManager::connectUDP(common::String* dest, common::uint16_t port, common::uint16_t localPort) {
  this->hasLink();
  uint32_t ip = this->parseIp(dest);
  if (this->udp == 0 || ip == 0) {
    return 0;
  }
  return this->udp->connect(ip, port, localPort);
}

bool NetworkManager::hasLink() {
  if (!this->foundDriver() || this->dhcp == 0) {
    return false;
  }
  bool link = this->currentDriver->hasLink();
  if (link && this->dhcp->shouldSend()) {
    this->dhcp->sendDiscover();
  }
  return link;
}

bool NetworkManager::foundDriver() {
  return this->currentDriver != 0;
}

String* NetworkManager::getIpString(String* dest) {
  char ret[16];
  uint32_t ip = this->parseIp(dest);
  for (int i = 0; i < 4; i++) {
    uint8_t ipPart = ip % 256;
    ret[(i*4)+3] = '.';
    ret[(i*4)+2] = '0' + (ipPart % 10);
    ret[(i*4)+1] = '0' + ((ipPart / 10) % 10);
    ret[(i*4)+0] = '0' + ((ipPart / 100) % 10);
    ip = ip >> 8;
  }
  ret[15] = 0;
  return new String(ret, 16);
}

uint32_t NetworkManager::parseIp(String* ip) {
  if (ip->occurrences('.') != 3) {
    return this->resolveIp(ip);
  }
  bool isIp = true;
  for (uint32_t i = 0; i < ip->getLength(); i++) {
    char c = ip->charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      isIp = false;
      break;
    }
  }
  if (!isIp) {
    return this->resolveIp(ip);
  }
  return this->stringToIp(ip);
}

uint32_t NetworkManager::stringToIp(String* ip) {
  String* ip1 = ip->split('.', 0);
  String* ip2 = ip->split('.', 1);
  String* ip3 = ip->split('.', 2);
  String* ip4 = ip->split('.', 3);
  uint32_t ip_BE = ((ip1->parseInt() & 0xFF))
              + ((ip2->parseInt() & 0xFF) << 8)
              + ((ip3->parseInt() & 0xFF) << 16)
              + ((ip4->parseInt() & 0xFF) << 24);
  delete ip4;
  delete ip3;
  delete ip2;
  delete ip1;
  return ip_BE;
}

uint32_t NetworkManager::resolveIp(String* ip) {
  if (this->dns == 0 || this->ipv4->getDomainServer() == 0) {
    return 0;
  }
  return this->dns->resolve(ip, this->ipv4->getDomainServer());
}
