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
    this->nodes[i].ampGainSteps = 0;
  }

  this->bdl_length = 4;
  this->bdl_start = 0;
}

HDA::~HDA() {}

void HDA::setupBDL() {
  uint32_t bufferSize = 0x10000;
  // INIT OUTPUT WIDGET
  uint16_t sample_rate = 0; // 0 = 48 khz.
  uint16_t num_channels = 0; // Channel is count - 1, so this is 1.
  uint16_t format = 16 | sample_rate | num_channels;

  this->corb_write_cmd(this->nodes[0].cad, this->nodes[0].nid, VERB_SET_STREAM_CHANNEL, 0x10);
  this->corb_write_cmd(this->nodes[0].cad, this->nodes[0].nid, VERB_SET_FORMAT, format);
  csr32(this->baseAddr, SDnCTL + SDnFMT) = format;


  this->bdl_start = (HDA_BDLE*) MemoryManager::activeMemoryManager->mallocalign(this->bdl_length * sizeof(HDA_BDLE), 128);
  for(int i = 0; i < this->bdl_length; i++) {
    this->bdl_start[i].length = bufferSize * sizeof(uint32_t);
    this->bdl_start[i].address = (uint64_t) MemoryManager::activeMemoryManager->mallocalign(this->bdl_start[i].length, 128);
    this->bdl_start[i].flags = 1;
    for(int w = 0; w < bufferSize; w++) { // Debug sqaure wave at 600 hz.
      uint32_t t = ((w / 8) % 2) == 0 ? 0 : -1;
      t &= 0xFFFFFF00;
      ((uint32_t*) this->bdl_start[i].address)[w] = t;
    }
  }

  csr8(this->baseAddr, SDnCTL + 2) = 0x10;
  csr32(this->baseAddr, SDnCTL + SDnCBL) = this->bdl_length * bufferSize * 4;
  csr16(this->baseAddr, SDnCTL + SDnLVI) = this->bdl_length - 1;

  csr32(this->baseAddr, SDnCTL + SDnBDPL) = (uint32_t) this->bdl_start;
  csr32(this->baseAddr, SDnCTL + SDnBDPU) = 0;

  csr32(baseAddr, Dplbase) = ((uint32_t) this->bdl_start) + 1;
	csr32(baseAddr, Dpubase) = 0;

  printf("Playing from cad: 0x");
  printHex32(this->nodes[0].cad);
  printf(" nid: 0x");
  printHex32(this->nodes[0].nid);
  printf("\n");

  csr16(this->baseAddr, SDnCTL) = 6;

  csr32(this->baseAddr, SSYNC) &= 0xFF000000;
  printf("SSYNC: ");
  printHex32(csr32(this->baseAddr, SSYNC));
  printf("\n");

  printf("\nPlaying: ");
  printHex32(csr32(this->baseAddr, SDnCTL));
  printf(" ");
  printHex8(csr8(this->baseAddr, SDnCTL));
  printf("\n");
  this->setVolume(255);
}

void HDA::setNode(int node) {
  uint32_t cad = this->nodes[0].cad;
  uint32_t nid = this->nodes[0].nid;
  uint32_t ampGainSteps = this->nodes[0].ampGainSteps;

  this->nodes[0].cad = this->nodes[node].cad;
  this->nodes[0].nid = this->nodes[node].nid;
  this->nodes[0].ampGainSteps = this->nodes[node].ampGainSteps;

  this->nodes[node].cad = cad;
  this->nodes[node].nid = nid;
  this->nodes[node].ampGainSteps = ampGainSteps;
}

void HDA::activate() {
  static int cmdbufsize[] = { 2, 16, 256, 2048 };

  uint8_t* baseAddr = this->baseAddr;

  csr8(baseAddr, Corbctl) = 0;
  csr8(baseAddr, Rirbctl) = 0;
  while(csr8(baseAddr, Corbctl) & Corbdma != 0) SystemTimer::sleep(50);
  while(csr8(baseAddr, Rirbctl) & Rirbdma != 0) SystemTimer::sleep(50);
  
  csr32(baseAddr, Gctl) = ~Rst;
  while(csr32(baseAddr, Gctl) & Rst != 0)
  SystemTimer::sleep(150);

  csr32(baseAddr, Gctl) |= Rst;
  while(csr32(baseAddr, Gctl) & Rst == 0) SystemTimer::sleep(50);

  csr16(baseAddr, WakeEn) = 0xFFFF;
  csr32(baseAddr, Intctl) = 0x800000FF;

  if(csr32(baseAddr, Statests) == 0) {
    printf("HDA: No codecs!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }

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
	// csr16(baseAddr, Statests) = csr16(baseAddr, Statests);
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

  SystemTimer::sleep(600);
  this->collectAFGNodes();
  this->setupBDL();
}

bool HDA::storeNode(uint32_t cad, uint32_t nid, uint32_t ampGainSteps) {
  for(int i = 0; i < this->maxNodes; i++) {
    if(this->nodes[i].cad == 0 && this->nodes[i].nid == 0) {
      this->nodes[i].cad = cad;
      this->nodes[i].nid = nid;
      this->nodes[i].ampGainSteps = ampGainSteps;
      return true;
    }
  }
  return false;
}

void HDA::collectAFGNodes() {
  uint32_t statests = csr32(baseAddr, Statests);
  for(int c = 0; c < 10; c++) {
    if((statests & (1 << c)) == 0) {
      continue;
    }
    uint64_t response = this->corb_write_cmd(c, 0, VERB_GET, 0x04);
    uint8_t startNode = (response >> 16) & 0xFF;
    uint8_t nodeCount = response & 0xFF;
    for(int n = startNode; n < startNode+nodeCount; n++) {
      response = this->corb_write_cmd(c, n, VERB_GET, 0x04)  & 0xFF;
      uint8_t startWidget = (response >> 16) & 0xFF;
      uint8_t widgetCount = response & 0xFF;
      uint8_t fg = this->corb_write_cmd(c, n, VERB_GET, 0x05) & 0xFF;

      if(fg == 0x01) { // AFG
        this->corb_write_cmd(c, n, VERB_SET_POWER_STATE, 0);
        for(int w = startWidget; w < startWidget+widgetCount; w++) {
          this->initWidget(c, w);
        }
      }
    }
  }
}

void HDA::initWidget(uint32_t cad, uint32_t nid) {
  uint32_t type = 0;
  uint32_t ampCap = 0;
  uint32_t eapdBtl = 0;
  uint32_t widgetCap = this->corb_write_cmd(cad, nid, VERB_GET, 0x09);

  if(widgetCap == 0) {
    return;
  }
  type = (widgetCap >> 20) & 0xF;

  ampCap = this->corb_write_cmd(cad, nid, VERB_GET, 18);

  eapdBtl = this->corb_write_cmd(cad, nid, VERB_GET_EAPD_BTL, 0);

  uint32_t pinCap, ctl; // WIDGET PIN
  switch (type) {
    case WIDGET_PIN:
      pinCap = this->corb_write_cmd(cad, nid, VERB_GET, 0x0C);
      printf("PIN CAP: ");
      printHex32(pinCap);
      printf("\n");

      if(pinCap & (1 << 4) == 0) {
        return;
      }
      
      ctl = this->corb_write_cmd(cad, nid, VERB_GET_PIN_CTL, 0);
      ctl |= 64; // OUTPUT ENABLE.
      this->corb_write_cmd(cad, nid, VERB_SET_PIN_CTL, ctl);

      eapdBtl |= 0x2;
      this->corb_write_cmd(cad, nid, VERB_SET_EAPD_BTL, eapdBtl);
      break;
    case WIDGET_OUTPUT:
      eapdBtl = eapdBtl | 0x2;
      this->corb_write_cmd(cad, nid, VERB_SET_EAPD_BTL, eapdBtl);

      this->storeNode(cad, nid, (ampCap >> 8) & 0x7F);
      printf("HDA: found output node cad=0x");
      printHex8(cad);
      printf(" nid=0x");
      printHex8(nid);
      printf("\n");
      break;
    default:
      return;
  }

  if((widgetCap & 0x400) != 0) {
    this->corb_write_cmd(cad, nid, VERB_SET_POWER_STATE, 1);
  }
}

uint64_t HDA::corb_write_cmd(uint32_t cad, uint32_t nid, uint32_t verb, uint32_t cmd) {
  uint64_t ret = 0;

  // Write command
  this->corb[csr16(this->baseAddr, Corbwp) + 1] = (cad << 28) | (nid << 20) | (verb << 8) | cmd;
  csr16(this->baseAddr, Corbwp) = (csr16(this->baseAddr, Corbwp) + 1) % this->corbSize;

  // Get response
  while(this->rirbRp > csr16(baseAddr, Rirbwp) && csr16(baseAddr, Rirbwp) == this->rirbRp-1) {
    SystemTimer::sleep(50);
  }
  ret = this->rirb[this->rirbRp];
  this->rirbRp = (this->rirbRp + 1) % this->rirbSize;
  return ret;
}

void HDA::setVolume(uint8_t volume) {
  uint32_t vol = volume * this->nodes[0].ampGainSteps / 255;
  vol |= 0xb000; // Output Amp, Left and Right
  vol &= ~0x80;
  printf("Vol: ");
  printHex32(vol);
  printf("\n");
  this->corb_write_cmd(this->nodes[0].cad, this->nodes[0].nid, VERB_SET_AMP_GAIN_MUTE, vol);
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
