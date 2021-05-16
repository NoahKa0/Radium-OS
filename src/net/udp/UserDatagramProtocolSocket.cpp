#include <net/udp/UserDatagramProtocolSocket.h>
#include <net/udp/UserDatagramProtocolProvider.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::net::udp;

UserDatagramProtocolHandler::UserDatagramProtocolHandler() {}
UserDatagramProtocolHandler::~UserDatagramProtocolHandler() {}
void UserDatagramProtocolHandler::handleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, uint8_t* data, uint32_t length) {}


UserDatagramProtocolSocket::UserDatagramProtocolSocket(UserDatagramProtocolProvider* backend) {
  this->backend = backend;
}

UserDatagramProtocolSocket::~UserDatagramProtocolSocket() {
  this->disconnect();
}

void UserDatagramProtocolSocket::handleUserDatagramProtocolMessage(uint8_t* data, uint32_t length) {
  if(handler != 0) {
    handler->handleUserDatagramProtocolMessage(this, data, length);
  }
}

void UserDatagramProtocolSocket::setHandler(UserDatagramProtocolHandler* handler) {
  this->handler = handler;
}

void UserDatagramProtocolSocket::send(common::uint8_t* data, common::uint16_t length) {
  backend->sendUDP(this, data, length);
}

void UserDatagramProtocolSocket::disconnect() {
  backend->disconnect(this);
}

void UserDatagramProtocolSocket::enableBroadcasts() {
  this->forwardBroadcasts = true;
}

void UserDatagramProtocolSocket::enableForwardAll() {
  this->forwardAll = true;
}

void UserDatagramProtocolSocket::setDestinationIp(uint32_t ip) {
  this->remoteIp = ip;
}
