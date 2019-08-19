#include <net/udp.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;

UserDatagramProtocolHandler::UserDatagramProtocolHandler() {}
UserDatagramProtocolHandler::~UserDatagramProtocolHandler() {}
void UserDatagramProtocolHandler::handleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, uint8_t* data, uint32_t length) {}


UserDatagramProtocolSocket::UserDatagramProtocolSocket(UserDatagramProtocolProvider* backend) {
  this->backend = backend;
}

UserDatagramProtocolSocket::~UserDatagramProtocolSocket() {}

void UserDatagramProtocolSocket::handleUserDatagramProtocolMessage(uint8_t* data, uint32_t length) {
  if(handler != 0) {
    handler->handleUserDatagramProtocolMessage(this, data, length);
  }
}

void UserDatagramProtocolSocket::send(common::uint8_t* data, common::uint16_t length) {
  backend->sendUDP(this, data, length);
}

void UserDatagramProtocolSocket::disconnect() {
  backend->disconnect(this);
}


UserDatagramProtocolProvider::UserDatagramProtocolProvider(InternetProtocolV4Provider* backend)
: InternetProtocolV4Handler(backend, 0x11)
{
  this->numSockets = 0;
  for(uint32_t = 0; i < 65535; i++) {
    sockets[i] = 0;
  }
  this->freePort = 1024;
}

UserDatagramProtocolProvider::~UserDatagramProtocolProvider() {}

void printf(char* txt);

bool UserDatagramProtocolProvider::onInternetProtocolReceived(uint32_t srcIp_BE, uint32_t destIp_BE, uint8_t* payload, uint32_t size) {
  if(size < sizeof(UserDatagramProtocolHeader)) return false;
  
  UserDatagramProtocolHeader* message = (UserDatagramProtocolHeader*) payload;
  uint16_t localPort = message->localPort;
  uint16_t remotePort = message->remotePort;
  
  UserDatagramProtocolSocket* socket = 0;
  for(uint16_t i = 0; i < numSockets && socket == 0; i++) {
    if(sockets[i]->localPort == message->destPort
    && sockets[i]->localIp == message->destIp
    && sockets[i]->remotePort == message->srcPort
    && sockets[i]->remoteIp == message->srcIp)
    {
      socket = sockets[i];
    } else {
      printf("Received unexpected UDP packet.\n");
    }
  }
  
  if(socket != 0) {
    socket->handleUserDatagramProtocolMessage(payload + sizeof(UserDatagramProtocolHeader), size - sizeof(UserDatagramProtocolHeader));
  }
  
  return false;
}

void UserDatagramProtocolProvider::sendUDP(UserDatagramProtocolSocket* socket, uint8_t* data, uint16_t length) {
  uint16_t totalLength = size + sizeof(UserDatagramProtocolHeader);
  uint8_t* buffer = MemoryManagement::activeMemoryManager->malloc(totalLength);
  uint8_t* payload = buffer + sizeof(UserDatagramProtocolHeader);
  UserDatagramProtocolHeader* message = (UserDatagramProtocolHeader*) buffer;
  
  message->srcPort = socket->localPort;
  message->destPort = socket->remotePort;
  message->length = totalLength;
  message->length = ((message->length & 0xFF00) >> 8) | ((message->length & 0x00FF) << 8);
  message->checksum = 0; // 0 means error checking is disabled.
  
  for(int i = 0; i < length; i++) {
    payload[i] = data[i];
  }
  
  this->send(socket->remoteIp, buffer, totalLength);
  
  MemoryManager::activeMemoryManager::free(buffer);
}

UserDatagramProtocolSocket* connect(uint32_t ip, uint16_t port) {
  UserDatagramProtocolSocket* socket = new UserDatagramProtocolSocket(this);
  
  socket->remotePort = port;
  socket->remoteIp = ip;
  socket->localPort = freePort;
  if(freePort == 65530) freePort = 1024;
  socket->localIp = backend->getIpAddress();
  
  socket->remotePort = ((socket->remotePort 0xFF00) >> 8) | ((socket->remotePort 0x00FF) << 8);
  socket->localPort = ((socket->localPort 0xFF00) >> 8) | ((socket->localPort 0x00FF) << 8);
  
  sockets[numSockets] = socket;
  numSockets++;
  
  return socket;
}

void disconnect(UserDatagramProtocolSocket* socket) {
  bool move = false;
  for(uint16_t i = 0; i < numSockets; i++) {
    if(sockets[i] == socket) {
      move = true;
      MemoryManager::activeMemoryManager::free(socket);
    }
    if(move) {
      sockets[i] = sockets[i+1];
    }
  }
  if(move) {
    numSockets--;
  }
}
