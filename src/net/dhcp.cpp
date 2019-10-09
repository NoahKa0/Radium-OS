#include <net/dhcp.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;

void printf(char* str);
void printHex32(uint32_t num);

DynamicHostConfigurationProtocol::DynamicHostConfigurationProtocol(UserDatagramProtocolProvider* udp, InternetProtocolV4Provider* ipv4)
:UserDatagramProtocolHandler()
{
  this->ipv4 = ipv4;
  this->magicCookie = 0x63538263; // Magic Cookie
  this->identifier = 302342478; // This number should be randomly generated.
  
  this->subnetmask = 0;
  this->defaultGateway = 0;
  this->ip = 0;
  this->lease = 0;
  this->ip_firstOffer = 0;
  
  this->socket = udp->connect(0xFFFFFFFF, 67, 68); // UDP connect to broadcast address on port 67, receive on port 68.
  this->socket->enableForwardAll(); // Enable forwarding off all UDP packets, even if their destination is not our IP (because we don't have one yet).
  this->socket->setHandler(this); // Forward packets to us.
}

DynamicHostConfigurationProtocol::~DynamicHostConfigurationProtocol() {
  this->socket->disconnect();
  MemoryManager::activeMemoryManager->free(this->socket);
  this->socket = 0;
}

void DynamicHostConfigurationProtocol::handleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, uint8_t* data, uint32_t length) {
  if(length < sizeof(DynamicHostConfigurationProtocolHeader)) return; // Our header size is the minimum required size, this message is invalid.
  DynamicHostConfigurationProtocolHeader* message = (DynamicHostConfigurationProtocolHeader*) data;
  
  if(message->optionsMagicCookie == this->magicCookie
    && message->identifier == this->identifier
    && message->operationCode == 2)
  {
    DynamicHostConfigurationOptionsInfo* info = this->getOptionsInfo((uint8_t*) message->options, 308); // Size should not be hard coded, but i don't think we will rereive that many options.
    
    switch(info->messageType) {
      case 2: // Offer
        this->handleOffer(message, info);
        break;
      case 5: // Acknowledge
      case 6: // NOT Acknowledge
        this->handleAck(message, info, (info->messageType == 6));
        break;
      default:
        break;
    }
    
    MemoryManager::activeMemoryManager->free(info); // Free memory used by info.
  }
}

void DynamicHostConfigurationProtocol::sendDiscover() {
  printf("DHCP Sending discover\n");
  // Set socket to use broadcast address.
  this->socket->setDestinationIp(0xFFFFFFFF);
  this->ip_firstOffer = 0;
  
  // Allocate memory for message
  uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(DynamicHostConfigurationProtocolHeader));
  for(uint32_t i = 0; i < sizeof(DynamicHostConfigurationProtocolHeader); i++) {
    data[i] = 0;
  }
  
  DynamicHostConfigurationProtocolHeader* message = (DynamicHostConfigurationProtocolHeader*) data;
  
  message->operationCode = 1; // 1 is request, 2 is reply.
  message->htype = 1; // 1 = 10mb ethernet.
  message->hlength = 6; // MAC Address = 6 bytes.
  message->hops = 0; // I don't know why, but the documentation says it should be 0.
  message->identifier = this->identifier; // Identifier, so i know wich messages are from me.
  
  message->secs = 0;
  message->flags = 0;
  
  message->clientAddress = this->ipv4->getIpAddress(); // This should be 0 unless we have an ip address (getIpAddress returns null when not assigned).
  message->myAddress = 0; // The server fills in this field, clients should not use it.
  message->serverAddress = 0; // We don't know this at this point.
  message->gatewayAddress = 0; // We don't know this at this point.
  
  message->clientHardwareAddress1 = this->ipv4->getMacAddress(); // MAC address.
  message->clientHardwareAddress2 = 0;
  
  message->optionsMagicCookie = this->magicCookie; // Magic Cookie.
  message->options[0] = 53; // Option value (message type).
  message->options[1] = 1; // Option length.
  message->options[2] = 1; // Discover.
  message->options[3] = 0; // Pad
  
  message->options[4] = 55; // Option value (request data).
  message->options[5] = 2; // Option length.
  message->options[6] = 1; // Subnet mask.
  message->options[7] = 3; // Default gateway (router).
  
  message->options[8] = 255; // End options.
  
  this->socket->send(data, sizeof(DynamicHostConfigurationProtocolHeader)); // Send data.
  MemoryManager::activeMemoryManager->free(data); // Mark memory used by message as free.
}

void DynamicHostConfigurationProtocol::handleOffer(DynamicHostConfigurationProtocolHeader* message, DynamicHostConfigurationOptionsInfo* info) {
  if(message->serverAddress == 0 || message->myAddress == 0 || this->ip_firstOffer != 0) return;
  
  // Save data.
  this->ip_firstOffer = message->serverAddress;
  this->ip = message->myAddress;
  this->lease = info->lease;
  this->defaultGateway = info->defaultGateway;
  this->subnetmask = info->subnetmask;
  
  printf("DHCP Offer, default gateway: ");
  printHex32(this->defaultGateway);
  printf("\n");
  
  // Send a request for this ip.
  this->sendRequest(message);
}

void DynamicHostConfigurationProtocol::sendRequest(DynamicHostConfigurationProtocolHeader* offer) {
  printf("DHCP Sending request\n");
  // Allocate memory for message
  uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(DynamicHostConfigurationProtocolHeader));
  for(uint32_t i = 0; i < sizeof(DynamicHostConfigurationProtocolHeader); i++) {
    data[i] = 0;
  }
  DynamicHostConfigurationProtocolHeader* message = (DynamicHostConfigurationProtocolHeader*) data;
  
  message->operationCode = 1; // 1 is request, 2 is reply.
  message->htype = 1; // 1 = 10mb ethernet.
  message->hlength = 6; // MAC Address = 6 bytes.
  message->hops = 0; // I don't know why, but the documentation says it should be 0.
  message->identifier = this->identifier; // Identifier, so i know wich messages are from me.
  
  message->secs = 0;
  message->flags = 0;
  
  message->clientAddress = this->ipv4->getIpAddress(); // This should be 0 unless we have an ip address (getIpAddress returns null when not assigned).
  message->myAddress = 0; // The server fills in this field, clients should not use it.
  message->serverAddress = offer->serverAddress;
  message->gatewayAddress = 0; // We don't know this at this point.
  
  message->clientHardwareAddress1 = this->ipv4->getMacAddress(); // MAC address.
  message->clientHardwareAddress2 = 0;
  
  message->optionsMagicCookie = this->magicCookie; // Magic Cookie.
  
  message->options[0] = 53; // Option value (message type).
  message->options[1] = 1; // Option length.
  message->options[2] = 3; // Discover.
  message->options[3] = 0; // Pad
  
  message->options[4] = 50; // Option value (requested ip address).
  message->options[5] = 4; // Option length.
  message->options[6] = (offer->myAddress) & 0xFF;
  message->options[7] = (offer->myAddress >> 8) & 0xFF;
  message->options[8] = (offer->myAddress >> 16) & 0xFF;
  message->options[9] = (offer->myAddress >> 24) & 0xFF;
  
  message->options[10] = 0; // Pad
  message->options[11] = 0; // Pad
  
  message->options[12] = 255; // End options.
  
  this->socket->send(data, sizeof(DynamicHostConfigurationProtocolHeader)); // Send data.
  
  MemoryManager::activeMemoryManager->free(data); // Mark memory used by message as free.
}

void DynamicHostConfigurationProtocol::handleAck(DynamicHostConfigurationProtocolHeader* message, DynamicHostConfigurationOptionsInfo* info, bool nak) {
  if(message->serverAddress == 0 || this->ip_firstOffer != message->serverAddress) return;
  if(nak) {
    printf("DHCP NAK!\n");
    return;
  }
  
  printf("DHCP complete: ");
  printHex32(this->ip);
  printf("\n");
  
  this->ipv4->setSubnetmask(this->subnetmask);
  this->ipv4->setGateway(this->defaultGateway);
  this->ipv4->setIpAddress(this->ip);
  
  this->ipv4->getArp()->broadcastMacAddress(0xFFFFFFFF);
  this->ipv4->getArp()->requestMacAddress(this->defaultGateway);
}

DynamicHostConfigurationOptionsInfo* DynamicHostConfigurationProtocol::getOptionsInfo(uint8_t* options, uint32_t size) {
  uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(sizeof(DynamicHostConfigurationOptionsInfo));
  for(uint32_t i = 0; i < sizeof(DynamicHostConfigurationOptionsInfo); i++) {
    data[i] = 0;
  }
  DynamicHostConfigurationOptionsInfo* info = (DynamicHostConfigurationOptionsInfo*) data;
  
  int i = 0;
  while(i < size-1) { // Options takes two bytes.
    uint8_t code = options[i];
    uint8_t length = options[i+1];
    if(code == 0) { // PAD, idk what pad is...
      i += 2;
    } else if(code == 255) { // END
      i = size;
    } else {
      this->handleOption(info, options+i+2, code, length);
      i += length+2;
    }
  }
  
  return info;
}

void DynamicHostConfigurationProtocol::handleOption(DynamicHostConfigurationOptionsInfo* info, uint8_t* option, uint8_t code, uint32_t size) {
  switch(code) {
    case 1: // Subnet mask
      if(size < 4) break;
      info->subnetmask = (option[3] << 24) | (option[2] << 16) | (option[1] << 8) | (option[0]);
      break;
    case 3: // Subnet mask
      if(size < 4) break;
      info->defaultGateway = (option[3] << 24) | (option[2] << 16) | (option[1] << 8) | (option[0]);
      break;
    case 51: // Subnet mask
      if(size < 4) break;
      info->lease = (option[3] << 24) | (option[2] << 16) | (option[1] << 8) | (option[0]);
      break;
    case 53: // Message type
      if(size != 1) break;
      info->messageType = option[0];
      break;
    default:
      break;
  }
}
