#include <net/dns.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::net::tcp;

void printf(const char* str);
void printHex32(uint32_t num);
void printNum(uint32_t num);

uint16_t SwapEndian16(uint16_t n);

DomainNameSystem::DomainNameSystem(TransmissionControlProtocolProvider* tcpProvider) {
  this->tcpProvider = tcpProvider;
  this->attempts = 4;
  this->lastIdentifier = 1;
  this->cacheSize = 256;
  this->cache = (CacheDescriptor*) MemoryManager::activeMemoryManager->malloc(this->cacheSize * sizeof(CacheDescriptor));
  this->cachePointer = 0;

  for(int i = 0; i < this->cacheSize; i++) {
    this->cache[i].answered = false;
    this->cache[i].discardAfter = 0;
    this->cache[i].identifier = 0;
    this->cache[i].ip = 0;
    this->cache[i].name = 0;
  }
}

DomainNameSystem::~DomainNameSystem() {
  delete this->cache;
}

uint32_t DomainNameSystem::resolve(String* domainName, uint32_t domainServer) {
  if (domainName->occurrences('.') == 0 || domainName->getLength() > 255) { // Domain names are never 1 part long.
    return 0;
  }

  for (int i = 0; i < this->cacheSize; i++) {
    CacheDescriptor desc = this->cache[i];
    if (desc.answered && desc.name->getLength() == domainName->getLength() && desc.name->equals(domainName->getCharPtr())) {
      return desc.ip;
    }
  }

  uint16_t id = this->lastIdentifier;
  this->lastIdentifier++;
  CacheDescriptor* cacheDescriptor = this->addToCache(id, domainName);

  for (int attempt = 0; attempt < this->attempts && !cacheDescriptor->answered; attempt++) {
    this->askServer(domainName, domainServer, cacheDescriptor);
  }
  cacheDescriptor->answered = true; // When all attemps fail, assume that the next try will fail.
  return cacheDescriptor->ip;
}

void DomainNameSystem::sendRequest(String* domainName, TransmissionControlProtocolSocket* socket, common::uint16_t id) {
  uint32_t partsTotalLength = 0;
  uint32_t partsCount = domainName->occurrences('.') + 1;
  String** parts = (String**) MemoryManager::activeMemoryManager->malloc(partsCount * sizeof(String*));
  for(int i = 0; i < partsCount; i++) {
    parts[i] = domainName->split('.', i);
    partsTotalLength += parts[i]->getLength() + 1;
  }
  partsTotalLength++;

  uint32_t totalLength = sizeof(DnsHeader) + partsTotalLength + 4; // 4 For question QTYPE and QCLASS.
  uint8_t* packet = (uint8_t*) MemoryManager::activeMemoryManager->malloc(totalLength + sizeof(uint16_t));
  packet[0] = (totalLength >> 8) & 0xFF;
  packet[1] = totalLength & 0xFF;
  DnsHeader* message = (DnsHeader*) (packet + 2);
  uint8_t* question = ((uint8_t*) message) + sizeof(DnsHeader);

  message->id = id;
  message->questionCount = SwapEndian16(1);
  message->answerCount = 0;
  message->nsCount = 0;
  message->additionalCount = 0;
  message->flags = SwapEndian16(0x100); // Recursion desired

  uint32_t questionTarget = 0;
  for(int i = 0; i < partsCount; i++) {
    String* part = parts[i];
    question[questionTarget] = part->getLength();
    questionTarget++;
    for(int j = 0; j < part->getLength(); j++) {
      question[questionTarget] = part->charAt(j);
      questionTarget++;
    }
  }
  question[questionTarget] = 0;
  question[questionTarget + 1] = 0; // TYPE
  question[questionTarget + 2] = 1;
  question[questionTarget + 3] = 0; // CLASS
  question[questionTarget + 4] = 1;

  socket->send(packet, totalLength + sizeof(uint16_t));

  delete packet;
  for(int i = 0; i < partsCount; i++) {
    delete parts[i];
  }
  delete parts;
}

DomainNameSystem::CacheDescriptor* DomainNameSystem::addToCache(uint16_t id, String* domainName) {
  uint32_t target = this->cachePointer;
  this->cachePointer = (this->cachePointer + 1) % this->cacheSize;

  if (this->cache[target].name != 0) {
    delete this->cache[target].name;
  }
  this->cache[target].answered = false;
  this->cache[target].discardAfter = 0;
  this->cache[target].identifier = id;
  this->cache[target].ip = 0;
  this->cache[target].name = new String(domainName->getCharPtr());

  return &this->cache[target];
}

uint32_t DomainNameSystem::parseAnswer(TransmissionControlProtocolSocket* socket, CacheDescriptor* cacheDescriptor) {
  uint8_t msgLength[2];
  socket->readNext(msgLength, 2);
  uint32_t length = SwapEndian16(((uint16_t*)msgLength)[0]);

  for(int i = 0; i < 50 && socket->isConnected() && socket->hasNext() < length; i++) {
    SystemTimer::sleep(100);
  }
  if(socket->hasNext() < length) {
    printf("DNS: Didn't receive full response: ");
    printNum(length);
    printf(", ");
    printNum(socket->hasNext());
    printf("\n");
    return 0;
  }

  if(length < sizeof(DnsHeader)) {
    printf("DNS: Invalid length received!\n");
    printNum(length);
    return 0;
  }
  // TODO Do proper checks, I will for now assume the answer is as expected.
  uint8_t* packet = (uint8_t*) MemoryManager::activeMemoryManager->malloc(length);
  socket->readNext(packet, length);
  DnsHeader* header = (DnsHeader*) packet;

  uint16_t questionCount = SwapEndian16(header->questionCount);
  uint16_t answerCount = SwapEndian16(header->answerCount);
  if (answerCount == 0) {
    cacheDescriptor->answered = true;
    cacheDescriptor->ip = 0;
    delete packet;
    return 0;
  }

  uint32_t readPtr = sizeof(DnsHeader);
  uint8_t* read = packet + readPtr;

  for(int i = 0; i < questionCount && readPtr < length; i++) {
    readPtr += this->getLabelLength(read, length - readPtr);
    readPtr += 4; // Skip over QTYPE(2) and QCLASS(2).
    
    read = packet + readPtr;
  }

  
  readPtr += this->getLabelLength(read, length - readPtr);
  // Skip over type(2) class(2) ttl(4)
  readPtr += 8;
  read = packet + readPtr;

  if(readPtr + 6 > length) {
    delete packet;
    return 0;
  }

  uint16_t addressLength = SwapEndian16(((uint16_t*) read)[0]);
  if(addressLength != 4) {
    delete packet;
    return 0;
  }

  uint32_t ip = (read[2]) | (read[3] << 8) | (read[4] << 16) | (read[5] << 24);

  cacheDescriptor->answered = true;
  cacheDescriptor->ip = ip;

  delete packet;

  return ip;
}

uint32_t DomainNameSystem::getLabelLength(uint8_t* label, uint32_t maxLength) {
  uint32_t ret = 0;
  while(ret < maxLength) {
    if (label[ret] == 0xC0) { // 16 bit pointer for compression.
      return ret + 2;
    }
    if (label[ret] == 0) {
      return ret + 1;
    }

    ret += label[ret] + 1;
  }
  return 0;
}

void DomainNameSystem::askServer(common::String* domainName, uint32_t domainServer, CacheDescriptor* cacheDescriptor) {
  TransmissionControlProtocolSocket* socket = this->tcpProvider->connect(domainServer, 53);
  if (socket == 0) {
    return;
  }

  for(int i = 0; i < 25 && !socket->isConnected(); i++) {
    SystemTimer::sleep(100);
  }
  if(!socket->isConnected()) {
    delete socket;
    return;
  }
  this->sendRequest(domainName, socket, cacheDescriptor->identifier);

  for(int i = 0; i < 25 && socket->hasNext() <= 2; i++) {
    SystemTimer::sleep(100);
  }
  if(socket->hasNext() <= 2) {
    delete socket;
    return;
  }
  
  this->parseAnswer(socket, cacheDescriptor);

  socket->disconnect();
  delete socket;
}
