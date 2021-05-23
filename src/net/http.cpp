#include <net/http.h>

using namespace sys;
using namespace sys::common;
using namespace sys::net;
using namespace sys::net::tcp;

uint16_t SwapEndian16(uint16_t n);

Http::Http(tcp::TransmissionControlProtocolSocket* socket) {
  this->socket = socket;
  this->responseCode = 0;
  this->responseSize = 0;
}

Http::~Http() {

}

void Http::request(String* host, String* query) {
  char* chars = query->getCharPtr();
  for(int i = 0; i < query->getLength(); i++) {
    if(chars[i] == ' ') {
      chars[i] = '-';
    }
  }
  String* request = new String("GET ");
  if (query->getLength() == 0 || chars[0] != '/') {
    request->append("/");
  }
  request->append(query->getCharPtr());
  request->append(" HTTP/1.1\r\nUser-Agent: Radium (Radium-OS)\r\nAccept: */*\r\n");

  request->append("Host: ");
  request->append(host->getCharPtr());
  request->append("\r\nConnection: keep-alive\r\n\r\n");
  
  this->socket->send((uint8_t*) request->getCharPtr(), request->getLength());
  delete request;

  this->getResponse();
}


void Http::getResponse() {
  char* line = (char*) MemoryManager::activeMemoryManager->malloc(512 * sizeof(char));
  uint32_t charPos = 0;
  uint32_t ignoredNewlines = 0;

  while((this->socket->isConnected() || this->socket->hasNext() != 0) && ignoredNewlines < 2) {
    for(uint32_t i = 0; i < 64 && this->socket->hasNext() == 0; i++) {
      this->socket->sendExpiredPackets();
      SystemTimer::sleep(150);
    }
    if (this->socket->hasNext() == 0) {
      break;
    }
    if (charPos < 512) {
      this->socket->readNext((uint8_t*) &(line[charPos]), 1);
      if (line[charPos] == '\n' || line[charPos] == '\r') {
        if (line[charPos] == '\n') {
          line[charPos] = 0;
          if (ignoredNewlines == 0 && charPos != 0) {
            this->processLine(line);
          }
          ignoredNewlines++;
          charPos = 0;
        }
      } else {
        ignoredNewlines = 0;
        charPos++;
      }
    } else {
      charPos = 0;
    }
  }
  delete line;
}

void Http::processLine(char* line) {
  uint32_t spaceCount = 0;
  String* str = new String(line);

  if (str->contains("HTTP/")) {
    this->responseCode = 0;
    for(int i = 0; i < str->getLength(); i++) {
      if (spaceCount == 1 && line[i] <= '9' && line[i] >= '0') {
        this->responseCode = (this->responseCode * 10) + (line[i] - '0');
      }
      if (line[i] == ' ') {
        spaceCount++;
      }
    }
  } else if (str->contains("Content-Length")) {
    this->responseSize = 0;
    for(int i = 0; i < str->getLength(); i++) {
      if (spaceCount == 1 && line[i] <= '9' && line[i] >= '0') {
        this->responseSize = (this->responseSize * 10) + (line[i] - '0');
      }
      if (line[i] == ' ') {
        spaceCount++;
      }
    }
  }
  delete str;
}

uint32_t Http::getResponseCode() {
  return this->responseCode;
}

uint32_t Http::getContentLength() {
  return this->responseSize;
}
