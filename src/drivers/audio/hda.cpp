#include <drivers/audio/hda.h>

using namespace sys;
using namespace sys::common;
using namespace sys::hardware;
using namespace sys::drivers;
using namespace sys::drivers::audio;

#define csr32(c,r) (*(uint32_t*) &(c)[r])
#define csr16(c,r) (*(uint16_t*) &(c)[r])
#define csr8(c,r) (*(uint8_t*) &(c)[r])

void printHex8(uint8_t);
void printHex32(uint32_t);
void printHex64(uint64_t);

void printf(const char*);

HDA::HDA(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager)
:Driver(),
InterruptHandler(device->interrupt + 0x20, interruptManager){
  if(device->memoryMapped != true) {
    printf("HDA bar is not set to memory mapped!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }
  this->device = device;

  this->baseAddr = (uint8_t*) device->addressBase;
  this->basePort = (uint32_t) device->portBase;

  // Enable MAC memory space decode and bus mastering.
  uint32_t pciCommandRead = this->device->read(0x04);
  pciCommandRead |= 0x06;
  this->device->write(0x04, pciCommandRead);

  this->corb = 0;

  for(int i = 0; i < this->maxNodes; i++) {
    this->nodes[i].cad = 0;
    this->nodes[i].nid = 0;
  }

  this->bdl_length = 4;
  this->bdl_start = 0;
}

HDA::~HDA() {}

void HDA::setupBDL() {
  this->bdl_start = (HDA_BDLE*) MemoryManager::activeMemoryManager->mallocalign(this->bdl_length * sizeof(HDA_BDLE), 128);
  for(int i = 0; i < this->bdl_length; i++) {
    this->bdl_start[i].length = 6000 * sizeof(uint32_t);
    this->bdl_start[i].address = (uint64_t) MemoryManager::activeMemoryManager->mallocalign(this->bdl_start[i].length, 128);
    this->bdl_start[i].IOC = 1;
    this->bdl_start[i].reserved1 = 1;
    this->bdl_start[i].reserved2 = 0;
    for(int w = 0; w < 6000; w++) { // Debug sqaure wave at 600 hz.
      uint32_t t = ((w / 4) % 2) == 0 ? 0 : -1;
      t &= 0xFFFFFF00;
      *((uint32_t*) this->bdl_start[w].address) = t;
    }
  }

  csr8(this->baseAddr, 0x83) &= ~0x20;
  printHex8(csr8(this->baseAddr, 0x83));

  csr8(this->baseAddr, SDnCTL) = 0; // Clear
  csr8(this->baseAddr, SDnCTL) = 0x01; // Reset
  SystemTimer::sleep(150);
  while(csr8(this->baseAddr, SDnCTL) & 0x01 != 0x01) SystemTimer::sleep(50);
  csr8(this->baseAddr, SDnCTL) &= ~0x01;
  SystemTimer::sleep(150);
  while(csr8(this->baseAddr, SDnCTL) & 0x01 != 0) SystemTimer::sleep(50);

  csr32(this->baseAddr, SDnCTL) = 1 << 20; // Set stream number (bit 20)

  csr32(this->baseAddr, SDnCBL) = this->bdl_length * 600 * 4;
  csr16(this->baseAddr, SDnFMT) = 0; // Default 48 kHz.
  csr16(this->baseAddr, SDnLVI) = this->bdl_length - 1;

  csr32(this->baseAddr, SDnBDPL) = (uint32_t) this->bdl_start;
  csr32(this->baseAddr, SDnBDPU) = 0;

  this->corb_write_cmd(0, 1, 0x707, 0b11100000);
  this->corb_write_cmd(0, 1, 0x706, 0b00010000);
  this->corb_write_cmd(0, 1, 0x3 | (0x40 << 4), 0x64);
  
  csr8(this->baseAddr, SDnCTL) |= 0b11100; // Enable interrupts
  csr8(this->baseAddr, SDnCTL) |= 0x10; // Start playing

  SystemTimer::sleep(5000);
  printf("Status: ");
  printHex8(csr8(this->baseAddr, 0x83));
  printf("\nPlaying: ");
  printHex32(csr32(this->baseAddr, SDnCTL));
  printf("\n");
}

void HDA::cmd_reset() {
  this->rirbRp = (csr16(baseAddr, Rirbwp) + 1) % this->rirbSize;
}

void HDA::activate() {
  static int cmdbufsize[] = { 2, 16, 256, 2048 };

  uint8_t* baseAddr = this->baseAddr;
  
  csr32(baseAddr, Gctl) &= ~Rst;
  SystemTimer::sleep(150);
  csr32(baseAddr, Gctl) |= Rst;
  while(csr32(baseAddr, Gctl) & Rst != Rst) SystemTimer::sleep(50);

  if(csr32(baseAddr, Statests) == 0) {
    printf("HDA: No codecs!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }

  csr8(baseAddr, Corbctl) = 0;
  while(csr8(baseAddr, Corbctl) & Corbdma != 0) SystemTimer::sleep(50);
  csr8(baseAddr, Rirbctl) = 0;
  while(csr8(baseAddr, Rirbctl) & Rirbdma != 0) SystemTimer::sleep(50);

  this->corbSize = cmdbufsize[csr8(baseAddr, Corbsz) & 0x3];
  this->corb = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(4 * this->corbSize, 128);
  for(int i = 0; i < this->corbSize; i++) {
    this->corb[i] = 0;
  }

  this->rirbSize = cmdbufsize[csr8(baseAddr, Rirbsz) & 0x3];
  this->rirb = (uint64_t*) MemoryManager::activeMemoryManager->mallocalign(8 * this->corbSize, 128);
  for(int i = 0; i < this->rirbSize; i++) {
    this->rirb[i] = 0;
  }

  /* setup controller  */
	csr32(baseAddr, Dplbase) = 0;
	csr32(baseAddr, Dpubase) = 0;
	csr16(baseAddr, Statests) = csr16(baseAddr, Statests);
	csr8(baseAddr, Rirbsts) = csr8(baseAddr, Rirbsts);

  csr32(baseAddr, Corblbase) = (uint32_t) this->corb;
	csr32(baseAddr, Corbubase) = 0;
	csr16(baseAddr, Corbwp) = 0;
	csr16(baseAddr, Corbrp) = Corbptrrst;
  while(csr16(baseAddr, Corbrp) & Corbptrrst != Corbptrrst) SystemTimer::sleep(50);
  csr16(baseAddr, Corbrp) = 0;
  while(csr16(baseAddr, Corbrp) & Corbptrrst != 0) SystemTimer::sleep(50);
  csr8(baseAddr, Corbctl) = Corbdma;
  while(csr8(baseAddr, Corbctl) & Corbdma != Corbdma) SystemTimer::sleep(50);

  this->rirbRp = 1;
  csr32(baseAddr, Rirblbase) = (uint32_t) this->rirb;
	csr32(baseAddr, Rirbubase) = 0;
	csr16(baseAddr, Rirbwp) = Rirbptrrst;
	csr8(baseAddr, Rirbctl) = Rirbdma;
  while(csr8(baseAddr, Rirbctl) & Rirbdma != Rirbdma) SystemTimer::sleep(50);

  csr8(baseAddr, Intctl) |= (1<<31) | (1<<30);

  this->collectAFGNodes();
  this->setupBDL();
}

bool HDA::storeNode(uint32_t cad, uint32_t nid) {
  for(int i = 0; i < this->maxNodes; i++) {
    if(this->nodes[i].cad == 0 && this->nodes[i].nid == 0) {
      this->nodes[i].cad = cad;
      this->nodes[i].nid = nid;
      return true;
    }
  }
  return false;
}

void HDA::collectAFGNodes() {
  for(int c = 0; c < 10; c++) {
    this->corb_write_cmd(c, 0, 0xF00, 0x04);
    uint64_t response = this->getNextSolicitedResponse();
    uint8_t startNode = (response >> 16) & 0xFF;
    uint8_t nodeCount = response & 0xFF;
    for(int n = startNode; n < startNode+nodeCount; n++) {
      this->corb_write_cmd(c, n, 0xF00, 0x05);
      uint8_t fg = this->getNextSolicitedResponse() & 0xFF;
      if(fg == 0x01) { // AFG
        this->corb_write_cmd(c, n, 0xF00, 0x0C);
        response = this->getNextSolicitedResponse();
        if(response != 0) {
          this->storeNode(c, n);
          printf("HDA: found node cad=0x");
          printHex8(c);
          printf(" nid=0x");
          printHex8(n);
          printf("\n");
        }
      }
    }
  }
}

uint64_t HDA::getNextSolicitedResponse() {
  uint64_t ret = 0;
  while(this->rirbRp > csr16(baseAddr, Rirbwp) && csr16(baseAddr, Rirbwp) == this->rirbRp-1) {
    SystemTimer::sleep(50);
  }
  ret = this->rirb[this->rirbRp];
  this->rirbRp = (this->rirbRp + 1) % this->rirbSize;
  return ret;
}

void HDA::corb_write_cmd(uint32_t cad, uint32_t nid, uint32_t verb, uint32_t cmd) {
  this->corb[csr16(this->baseAddr, Corbwp) + 1] = (cad << 28) | (nid << 20) | (verb << 8) | cmd;
  csr16(this->baseAddr, Corbwp) = (csr16(this->baseAddr, Corbwp) + 1) % this->corbSize;
}

int HDA::reset() {
  return 0;
}

void HDA::deactivate() {}

uint32_t HDA::handleInterrupt(uint32_t esp) {
  printf("HDA INT!\n");
  return esp;
}

Driver* HDA::getDriver(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager) {
  Driver* ret = 0;

  if(device->classId == 0x04 && device->subclassId == 0x03 && device->memoryMapped == true) {
    ret = new HDA(device, interruptManager);
  }

  return ret;
}
