#include <cli/wbmp.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;
using namespace sys::drivers;

void printf(const char*);
void printNum(uint32_t);
void enableVGA();
void disableVGA();
VideoGraphicsArray* getVGA();

CmdWBMP::CmdWBMP() {
  this->pressedEnter = 0;
}
CmdWBMP::~CmdWBMP() {
}

void CmdWBMP::showBmp(common::uint8_t* data, common::uint32_t length) {
  if (length < sizeof(BMPFileHeader) + sizeof(DIBHeader)) {
    return;
  }
  // --- BMP signature ---
  if (data[0] != 'B' || data[1] != 'M') {
      return; // not a BMP
  }

  BMPFileHeader* bmpHeader = (BMPFileHeader*) data;
  DIBHeader* dib = (DIBHeader*) (data + sizeof(BMPFileHeader));

  if (dib->bitCount != 24 || dib->compression != 0) {
    return; // only support 24-bit uncompressed
  }

  int width  = dib->width;
  int height = dib->height;

  bool bottomUp = true;
  if (height < 0) {
    height = -height;
    bottomUp = false;
  }

  int bytesPerPixel = 3;
  int rowSize = ((width * bytesPerPixel + 3) / 4) * 4;

  common::uint8_t* pixelData = data + bmpHeader->bfOffBits;

  enableVGA();

  VideoGraphicsArray* vga = getVGA();
  // --- Draw ---
  for (int y = 0; y < height; y++) {
    int srcY = bottomUp ? (height - 1 - y) : y;

    common::uint8_t* row = pixelData + srcY * rowSize;

    for (int x = 0; x < width; x++) {
      common::uint8_t b = row[x * 3 + 0];
      common::uint8_t g = row[x * 3 + 1];
      common::uint8_t r = row[x * 3 + 2];

      int vx = (x*320)/width;
      int vy = (y*200)/height;

      vga->putPixel(vx, vy, r, g, b);
    }
  }
}

void CmdWBMP::run(common::String** args, common::uint32_t argsLength) {
  bool connectionFailed = false;
  uint32_t responseCode = 0;
  uint32_t contentLength = 0;
  uint32_t bytesReceived = 0;

  if(argsLength < 2) {
    printf("Usage: <host> <query>\n");
    return;
  }

  String* ip = NetworkManager::networkManager->getIpString(args[0]);
  String* query = new String(args[1]->getCharPtr());
  printf("Connecting to: ");
  printf(ip->getCharPtr());
  printf("... ");

  this->socket = NetworkManager::networkManager->connectTCP(ip, 80);
  if (this->socket == 0) {
    printf("\nCould not resolve: ");
    printf(args[0]->getCharPtr());
    printf("\n");
    delete query;
    delete ip;
    return;
  }

  for(int i = 0; i < 10 && !this->socket->isConnected(); i++) {
    SystemTimer::sleep(1000);
  }

  if(this->socket->isConnected()) {
    printf("Connected!\n");
    Http* httpClient = new Http(this->socket);
    httpClient->request(args[0], query);
    responseCode = httpClient->getResponseCode();
    contentLength = httpClient->getContentLength();
    delete httpClient;
  } else {
    printf("failed!\n");
    connectionFailed = true;
  }

  if (contentLength < 1) {
    contentLength = 1;
  }

  uint64_t waitUntil = 0;
  uint32_t secondsPassed = 1;

  uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(contentLength);

  if (responseCode == 200) {
    while((this->socket->isConnected() || this->socket->hasNext() != 0) && this->pressedEnter < 6 && bytesReceived < contentLength) {
      if(this->socket->hasNext() != 0) {
        uint32_t bytesToRead = this->socket->hasNext();
        if (bytesToRead > (contentLength - bytesReceived)) {
          bytesToRead = (contentLength - bytesReceived);
        }
        this->socket->readNext(data + bytesReceived, bytesToRead);
        bytesReceived += bytesToRead;
        if (SystemTimer::getTimeInInterrupts() > waitUntil) {
          uint32_t secondsLeft = (contentLength - bytesReceived) * secondsPassed / bytesReceived;
          printf("Received ");
          printNum(bytesReceived * 100 / contentLength);
          printf("%, ");
          printNum(secondsLeft / 60);
          printf(" minutes and ");
          printNum(secondsLeft % 60);
          printf(" seconds left\n");
          waitUntil = SystemTimer::getTimeInInterrupts() + SystemTimer::millisecondsToLength(1000);
          secondsPassed++;
        }
      } else {
        this->socket->sendExpiredPackets();
      }
      SystemTimer::sleep(10);
    }
  } else {
    printf("HTTP failed: ");
    printNum(responseCode);
    printf("\n");
  }

  if(!connectionFailed) {
    for(int i = 0; i < 10 && !this->socket->isClosed(); i++) {
      SystemTimer::sleep(1000);
    }
  }

  delete query;
  delete ip;
  delete this->socket;
  this->socket = 0;

  if (bytesReceived < contentLength && responseCode == 200) {
    printf("Download cancelled by user!\n");
    delete data;
    return;
  }

  showBmp(data, contentLength);

  delete data;
}

void CmdWBMP::onInput(common::String* input) {
  this->pressedEnter++;
  printf("\n");
}
