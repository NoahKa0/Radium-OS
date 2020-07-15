#include <drivers/net/broadcom_BCM5751.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);
void printHex32(uint32_t num);
void printHex8(uint8_t num);
void setSelectedEthernetDriver(EthernetDriver* drv);

#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))
#define Rbsz		ROUNDUP(sizeof(Etherpacket)+4, 4)
#define csr32(c, r)	((c)[(r)/4])
#define BLEN(s)	((s)->wp - (s)->rp)

Block* broadcom_BCM5751::allocb(common::uint32_t size) {
  Block *b;
  uint32_t* addr;
  
  // The 16 and 64 variables come from Hdrspc = 64, Tlrspc = 16, in original driver.
  size += 16;
  b = (Block*) MemoryManager::activeMemoryManager->mallocalign(sizeof(Block)+size+(64)+32, 32);
  
  if(b == 0) {
    printf("BCM5751->allocb->malloc failed!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }

  b->next = 0;
  
  /*
    NOTICE I couldn't find a struct Block that contained list, free or flag. I seems as if they aren't used here anyway, so i don't think it will be a problem.
    
    b->list = 0;
    b->free = 0;
    b->flag = 0;
  */
  
  
  addr = (uint32_t*) b;
  addr = addr + sizeof(Block);
  b->base = (uint8_t*) addr;
  
  b->lim = (uint8_t*) b + (sizeof(Block)+size+(64)+32);

  // leave space for added headers
  b->rp = b->lim - size;
  uint32_t mod = ((uint32_t) b->rp) % 32;
  if(mod != 0) {
    b->lim -= mod;
    b->rp -= mod;
  }
  if(b->rp < b->base) {
    printf("BCM5751->allocb rp < base!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }
  b->wp = b->rp;

  return b;
}

void broadcom_BCM5751::bcmtransclean() {
  while(this->ctlr.sendcleani != (this->ctlr.status[4] >> 16)) {
		MemoryManager::activeMemoryManager->free(this->ctlr.sends[this->ctlr.sendcleani]);
		this->ctlr.sends[this->ctlr.sendcleani] = 0;
		this->ctlr.sendcleani = (this->ctlr.sendcleani + 1) & (TxRingLen - 1);
	}
}

int32_t broadcom_BCM5751::miir(uint32_t* nic, int32_t ra) {
  while(csr32(nic, MIComm) & (1<<29));
  csr32(nic, MIComm) = (ra << 16) | (1 << 21) | (1 << 27) | (1 << 29);
  while(csr32(nic, MIComm) & (1<<29));
  if(csr32(nic, MIComm) & (1<<28)) return -1;
  return csr32(nic, MIComm) & 0xFFFF;
}

int32_t broadcom_BCM5751::miiw(uint32_t* nic, int32_t ra, int32_t value) {
  while(csr32(nic, MIComm) & (1<<29));
  csr32(nic, MIComm) = (value & 0xFFFF) | (ra << 16) | (1 << 21) | (1 << 27) | (1 << 29);
  while(csr32(nic, MIComm) & (1<<29));
  return 0;
}

common::int32_t broadcom_BCM5751::replenish(Block* bp) {
  uint32_t* next;
  uint32_t incr;
  
  incr = (this->ctlr.recvprodi + 1) & (RxProdRingLen - 1);
  if(incr == (this->ctlr.status[2] >> 16)) return -1;
  if(bp == 0) {
    bp = this->allocb(Rbsz);
  }
  if(bp == 0) {
    printf("bcm: out of memory for receive buffers\n");
    return -1;
  }
  next = this->ctlr.recvprod + this->ctlr.recvprodi * 8;
  memset(next, 0, 32);
  next[0] = 0;
  next[1] = this->paddr((uint32_t) bp->rp);
  next[2] = Rbsz;
  next[7] = this->ctlr.recvprodi;
  this->ctlr.rxs[this->ctlr.recvprodi] = bp;
  csr32(this->ctlr.nic, RxProdBDRingIdx) = this->ctlr.recvprodi = incr;
  return 0;
}

uint64_t broadcom_BCM5751::dummyread(uint64_t n) {
  return n;
}

void broadcom_BCM5751::memset(void* dst, int32_t v, int32_t n) {
  uint8_t* d = (uint8_t*) dst;
  
  while(n > 0) {
    *d = v;
    d++;
    n--;
  }
}

uint64_t broadcom_BCM5751::paddr(uint64_t a) {
  return (a) & ~0xFFFFFFFFF0000000ull;
}

uint32_t* broadcom_BCM5751::currentrecvret() {
	if(this->ctlr.recvreti == (this->ctlr.status[4] & 0xFFFF)) return 0;
	return this->ctlr.recvret + this->ctlr.recvreti * 8;
}

void broadcom_BCM5751::consumerecvret() {
  this->ctlr.recvreti = this->ctlr.recvreti+1 & RxRetRingLen-1;
	csr32(this->ctlr.nic, RxBDRetRingIdx) = this->ctlr.recvreti = this->ctlr.recvreti;
}

void broadcom_BCM5751::checklink() {
  uint32_t i;
  uint32_t* nic = this->ctlr.nic;
  
  miir(nic, Bmsr); /* Dummy read */
	if(!(miir(nic, Bmsr) & 4)) {
    this->link = 0;
    this->mbps = 1000;
    this->ctlr.duplex = 1;
    printf("bcm: no link\n");
    goto out;
	}
  this->link = 1;
  while((miir(nic, Bmsr) & 32) == 0);
  i = miir(nic, Mssr);
  if(i & (Mssr1000THD | Mssr1000TFD)) {
    this->mbps = 1000;
    this->ctlr.duplex = (i & Mssr1000TFD) != 0;
  } else if(i = miir(nic, Anlpar), i & (AnaTXHD | AnaTXFD)) {
    this->mbps = 100;
    this->ctlr.duplex = (i & AnaTXFD) != 0;
  } else if(i & (Ana10HD | Ana10FD)) {
    this->mbps = 10;
    this->ctlr.duplex = (i & Ana10FD) != 0;
  } else {
    this->link = 0;
    this->mbps = 1000;
    this->ctlr.duplex = 1;
    printf("bcm: link partner supports neither 10/100/1000 Mbps\n"); 
    goto out;
  }
  printf("bcm: ");
  printHex32(this->mbps);
  printf("Mbps ");
  if(this->ctlr.duplex) {
    printf("full ");
  } else {
    printf("half ");
  }
  printf("duplex\n");
out:
  if(this->ctlr.duplex) csr32(nic, MACMode) &= ~MACHalfDuplex;
  else csr32(nic, MACMode) |= MACHalfDuplex;
  if(this->mbps >= 1000) {
    csr32(nic, MACMode) = (csr32(nic, MACMode) & MACPortMask) | MACPortGMII;
  } else {
    csr32(nic, MACMode) = (csr32(nic, MACMode) & MACPortMask) | MACPortMII;
  }
  csr32(nic, MACEventStatus) |= (1<<4) | (1<<3); /* undocumented bits (sync and config changed) */
}

broadcom_BCM5751::broadcom_BCM5751(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager)
: EthernetDriver(),
InterruptHandler(device->interrupt + 0x20, interruptManager) // hardware interrupt is asigned, so the device doesn't know that it starts at 0x20.
{
  this->device = device;
  if(device->memoryMapped != true) {
    printf("BCM5751 bar is not set to memory mapped!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }

  this->ipAddr = 0;

  // Enable MAC memory space decode and bus mastering.
  uint32_t pciCommandRead = this->device->read(0x04);
  pciCommandRead |= 0x06;
  this->device->write(0x04, pciCommandRead);
  
  this->macAddress = 0;
  this->mbps = 0;
  this->link = 0;
  
  this->ctlr.nic = (uint32_t*) device->addressBase;
  this->ctlr.port = (uint32_t) device->portBase;
  
  this->ctlr.sendcleani = 0;
  this->ctlr.sendri = 0;
  
  this->ctlr.status = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(20+16, 16);
  this->ctlr.recvprod = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(32 * RxProdRingLen + 16, 16);
  this->ctlr.recvret = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(32 * RxRetRingLen + 16, 16);
  this->ctlr.sendr = (uint32_t*) MemoryManager::activeMemoryManager->mallocalign(16 * TxRingLen + 16, 16);
  
  this->ctlr.sends = (Block**) MemoryManager::activeMemoryManager->malloc(sizeof(this->ctlr.sends[0]) * TxRingLen);
  this->ctlr.rxs = (Block**) MemoryManager::activeMemoryManager->malloc(sizeof(this->ctlr.sends[0]) * TxRingLen);

  sendBuffer = (Block**) MemoryManager::activeMemoryManager->malloc(sizeof(this->ctlr.sends[0]) * TxRingLen); 
  for(int i = 0; i < TxRingLen; i++) sendBuffer[i] = 0;
  sendRead = 0;
  sendWrite = 0;
  
  setSelectedEthernetDriver(this); // Make this instance accessable in kernel.cpp
}

broadcom_BCM5751::~broadcom_BCM5751() {}

void broadcom_BCM5751::activate() {
  uint64_t i, j;
  uint32_t* nic = this->ctlr.nic;
  uint32_t* mem = nic + 0x2000;
  
  csr32(nic, MiscHostCtl) |= MaskPCIInt | ClearIntA | WordSwap | IndirAccessEn;
  csr32(nic, SwArbit) |= SwArbitSet1;
  
  while((csr32(nic, SwArbit) & SwArbitWon1) == 0) {
    SystemTimer::sleep(40);
  }
  
	csr32(nic, MemArbiterMode) |= Enable;
	csr32(nic, MiscHostCtl) = WordSwap | IndirAccessEn | PCIStateRegEn | EnableClockCtl
		| MaskPCIInt | ClearIntA;
	csr32(nic, Memwind) = 0;
  csr32(mem, 0xB50) = 0x4B657654; // magic number
  
  csr32(nic, MiscConf) |= GPHYPwrdnOverride | DisableGRCRstOnPpcie | CoreClockBlocksReset;
  
  SystemTimer::sleep(150); // I should wait 100 ms, but the timer isn't that accurate, so wait slightly more.
  
  // Enable MAC memory space decode and bus mastering.
  uint32_t pciCommandRead = this->device->read(0x04);
  pciCommandRead |= 0x06; // 0x04 | 0x02
  this->device->write(0x04, pciCommandRead);
  
	csr32(nic, MiscHostCtl) |= MaskPCIInt | ClearIntA;
	csr32(nic, MemArbiterMode) |= Enable;
	csr32(nic, MiscHostCtl) |= WordSwap | IndirAccessEn | PCIStateRegEn | EnableClockCtl | TaggedStatus;
	csr32(nic, ModeControl) |= ByteWordSwap;
	csr32(nic, MACMode) = (csr32(nic, MACMode) & MACPortMask) | MACPortGMII;
  
  SystemTimer::sleep(100); // Original driver sleeps for 40 ms, but the timer isn't that accurate.
  
  while(csr32(mem, 0xB50) != 0xB49A89AB) {
    SystemTimer::sleep(100);
  }
  
  this->memset(this->ctlr.status, 0, 20);


  csr32(nic, Dmarwctl) = (csr32(nic, Dmarwctl) & DMAWaterMask) | DMAWaterValue;
	csr32(nic, ModeControl) |= HostTxBDs | HostStackUp | InterruptOnMAC;
	csr32(nic, MiscConf) = (csr32(nic, MiscConf) & TimerMask) | TimerValue;
	csr32(nic, MBUFLowWater) = 0x20;
	csr32(nic, MBUFHighWater) = 0x60;
	csr32(nic, LowWaterMax) = (csr32(nic, LowWaterMax) & LowWaterMaxMask) | LowWaterMaxValue;
	csr32(nic, BufferManMode) |= Enable | Attn;

  while((csr32(nic, BufferManMode) & Enable) == 0);
  
	csr32(nic, FTQReset) = ~0;
	csr32(nic, FTQReset) = 0;

  while(csr32(nic, FTQReset));
  
	csr32(nic, RxBDHostAddr) = 0;
	csr32(nic, RxBDHostAddr + 4) = paddr((uint64_t) this->ctlr.recvprod);
	csr32(nic, RxBDFlags) = RxProdRingLen << 16;
	csr32(nic, RxBDNIC) = 0x6000;
	csr32(nic, RxBDRepl) = 25;
	csr32(nic, TxBDRingHostIdx) = 0;
	csr32(nic, TxBDRingHostIdx+4) = 0;
  csr32(mem, TxRCB) = 0;
  csr32(mem, TxRCB + 4) = paddr((uint64_t) this->ctlr.sendr);
  csr32(mem, TxRCB + 8) = TxRingLen << 16;
  csr32(mem, TxRCB + 12) = 0x4000;
  
  for(i=1;i<4;i++)
    csr32(mem, RxRetRCB + i * 0x10 + 8) = 2;
  csr32(mem, RxRetRCB) = 0;
  csr32(mem, RxRetRCB + 4) = paddr((uint64_t) this->ctlr.recvret);
  csr32(mem, RxRetRCB + 8) = RxRetRingLen << 16;
  csr32(nic, RxProdBDRingIdx) = 0;
  csr32(nic, RxProdBDRingIdx+4) = 0;
  
  asm("sti");
  SystemTimer::sleep(100);
  asm("cli");
  
  // NOTICE this mess is because i messed up how the mac address is send, i should clean this up.
  i = csr32(nic, 0x410); // I don't know what this does, i also don't know why it is stored with the address.
  this->macAddress = ((i & 0x00FF) << 8) | ((i & 0xFF00) >> 8);
  this->macAddress = this->macAddress << 16;
  
  i = csr32(nic, MACAddress + 4);
  this->macAddress |= (i & 0x00000000000000FF) << 56;
  this->macAddress |= (i & 0x000000000000FF00) << 40;
  this->macAddress |= (i & 0x0000000000FF0000) << 24;
  this->macAddress |= (i & 0x00000000FF000000) << 8;
  this->macAddress = this->macAddress >> 16;
  
	csr32(nic, RandomBackoff) = j & 0x3FF;
	csr32(nic, RxMTU) = Rbsz;
	csr32(nic, TxMACLengths) = 0x2620;
	csr32(nic, RxListPlacement) = 1<<3; /* one list */
	csr32(nic, RxListPlacementMask) = 0xFFFFFF;
	csr32(nic, RxListPlacementConf) |= RxStats;
	csr32(nic, TxInitiatorMask) = 0xFFFFFF;
	csr32(nic, TxInitiatorConf) |= TxStats;
	csr32(nic, HostCoalMode) = 0;
  while(csr32(nic, HostCoalMode) != 0);
	csr32(nic, HostCoalRxTicks) = 150;
	csr32(nic, HostCoalTxTicks) = 150;
	csr32(nic, RxMaxCoalFrames) = 10;
	csr32(nic, TxMaxCoalFrames) = 10;
	csr32(nic, RxMaxCoalFramesInt) = 0;
	csr32(nic, TxMaxCoalFramesInt) = 0;
  csr32(nic, StatusBlockHostAddr) = 0;
  csr32(nic, StatusBlockHostAddr + 4) = paddr((uint64_t) this->ctlr.status);
	csr32(nic, HostCoalMode) |= Enable;
	csr32(nic, RxBDCompletionMode) |= Enable | Attn;
	csr32(nic, RxListPlacementMode) |= Enable;
	csr32(nic, MACMode) |= MACEnable;
	csr32(nic, MiscLocalControl) |= InterruptOnAttn | AutoSEEPROM;
	csr32(nic, InterruptMailbox) = 0;
	csr32(nic, WriteDMAMode) |= 0x200003fe; /* pulled out of my nose */
	csr32(nic, ReadDMAMode) |= 0x3fe;
	csr32(nic, RxDataCompletionMode) |= Enable | Attn;
	csr32(nic, TxDataCompletionMode) |= Enable;
	csr32(nic, TxBDCompletionMode) |= Enable | Attn;
	csr32(nic, RxBDInitiatorMode) |= Enable | Attn;
	csr32(nic, RxDataBDInitiatorMode) |= Enable | (1<<4);
	csr32(nic, TxDataInitiatorMode) |= Enable;
	csr32(nic, TxBDInitiatorMode) |= Enable | Attn;
	csr32(nic, TxBDSelectorMode) |= Enable | Attn;
  this->ctlr.recvprodi = 0;
  
  while(this->replenish() >= 0);
  
	csr32(nic, TxMACMode) |= Enable;
	csr32(nic, RxMACMode) |= Enable;
	csr32(nic, Pwrctlstat) &= ~3;
	csr32(nic, MIStatus) |= 1<<0;
	csr32(nic, MACEventEnable) = 0;
	csr32(nic, MACEventStatus) |= (1<<12);
	csr32(nic, MIMode) = 0xC0000;		/* set base mii clock */
  
  asm("sti");
  SystemTimer::sleep(100);
  asm("cli");
  
  this->miiw(nic, Bmcr, 1<<15);
  while(this->miir(nic, Bmcr) & (1<<15));
	miiw(nic, Bmcr, 4096 | 512);

	miiw(nic, PhyAuxControl, 2);
	miir(nic, PhyIntStatus);
	miir(nic, PhyIntStatus);
	miiw(nic, PhyIntMask, ~(1<<1));
  
  checklink();
  
	csr32(nic, MACEventEnable) |= 1<<12;
	for(i = 0; i < 4; i++) {
    csr32(nic, MACHash + 4*i) = ~0;
  }
	for(i = 0; i < 8; i++) {
    csr32(nic, RxRules + 8 * i) = 0;
  }
	csr32(nic, RxRulesConf) = 1 << 3;
	csr32(nic, MSIMode) |= Enable;
	csr32(nic, MiscHostCtl) &= ~(MaskPCIInt | ClearIntA);
  
  asm("sti");
}

int broadcom_BCM5751::reset() {
  // TODO
  return 0;
}

common::uint32_t broadcom_BCM5751::handleInterrupt(common::uint32_t esp) {
  uint32_t* nic = this->ctlr.nic;
  uint64_t status = 0;
  uint64_t tag = 0;
  
	broadcom_BCM5751::dummyread(csr32(nic, InterruptMailbox));
	csr32(nic, InterruptMailbox) = 1;
	status = this->ctlr.status[0];
	tag = this->ctlr.status[1];
	this->ctlr.status[0] = 0;
  
  // Error.
	if(status & Error) {
    if(csr32(nic, FlowAttention)) {
      if(csr32(nic, FlowAttention) & 0xF8FF8080UL) {
        printf("bcm: fatal error!");
        while(true) {
          asm("cli");
          asm("hlt");
        }
      }
      csr32(nic, FlowAttention) = 0;
    }
    csr32(nic, MACEventStatus) = 0; /* worth ignoring */
    if(csr32(nic, ReadDMAStatus) || csr32(nic, WriteDMAStatus)) {
      printf("bcm: dma error!\n");
      csr32(nic, ReadDMAStatus) = 0;
      csr32(nic, WriteDMAStatus) = 0;
    }
    if(csr32(nic, RISCState)) {
      if(csr32(nic, RISCState) & 0x78000403) {
        printf("bcm: RISC halted!");
        while(true) {
          asm("cli");
          asm("hlt");
        }
      }
      csr32(nic, RISCState) = 0;
    }
  }
  
	if(status & LinkStateChange) this->checklink();
	this->receive();
  this->bcmtransclean();
  //this->transmit();
	csr32(nic, InterruptMailbox) = tag << 24;
  
  return esp;
}

void broadcom_BCM5751::send(common::uint8_t* buffer, int size) {
  if((sendWrite+1) % TxRingLen == sendRead) { // Send buffer full
    return;
  }

  if(size > sizeof(Etherpacket)) {
    size = sizeof(Etherpacket);
  }

  Block* bp = this->allocb(Rbsz);
  if(bp == 0) return;
  bp->wp = bp->rp + size;
  uint8_t* blockData = bp->rp;
  for(int i = 0; i < size; i++) {
    blockData[i] = buffer[i];
  }

  sendBuffer[sendWrite] = bp;

  sendWrite = (sendWrite + 1) % TxRingLen;
  
  this->transmit();
}

void broadcom_BCM5751::transmit() {
  uint64_t incr = 0;
  uint32_t* next = 0;
  Block* bp = 0;

  while(true) {
    incr = (this->ctlr.sendri + 1) & (TxRingLen - 1);
    if(incr == this->ctlr.sendcleani) {
      printf("bcm: send queue full\n");
      break;
    }

    bp = sendBuffer[sendRead];
    if(bp == 0) {
      break;
    }
    sendBuffer[sendRead] = 0;
    sendRead = (sendRead + 1) % TxRingLen;

    next = this->ctlr.sendr + this->ctlr.sendri * 4;
    next[0] = 0;
    next[1] = this->paddr((uint32_t) bp->rp);
    next[2] = ((bp->wp - bp->rp) << 16) | PacketEnd;
    next[3] = 0;
    this->ctlr.sends[this->ctlr.sendri] = bp;

    this->ctlr.sendri = incr;
    csr32(this->ctlr.nic, TxBDRingHostIdx) = this->ctlr.sendri;
  }
}

void broadcom_BCM5751::receive() {
  Block* bp;
  uint32_t* pkt;
  uint32_t len;
  
  while(pkt = currentrecvret()) {
		bp = this->ctlr.rxs[pkt[7]];

    if(bp == 0) {
			break;
		}
    
    len = pkt[2] & 0xFFFF;
    bp->wp = bp->rp + len;
    if((pkt[3] & PacketEnd) == 0) printf("bcm: partial frame received\n");
    if(!(pkt[3] & FrameError)) {

      if(this->etherFrameProvider != 0) {
        if(this->etherFrameProvider->onRawDataRecived(bp->rp, len)) {
          send(bp->rp, len);
        }
      }
    }
    // NOTICE code below should not be used, since we reuse packets.
    // else freeb(bp);
    
    this->replenish(bp); // Reuse old block.
    this->consumerecvret();
  }
}

uint64_t broadcom_BCM5751::getMacAddress() {
  return this->macAddress;
}

uint32_t broadcom_BCM5751::getIpAddress() {
  return this->ipAddr;
}

bool broadcom_BCM5751::hasLink() {
  return this->link != 0;
}

void broadcom_BCM5751::setIpAddress(uint32_t ip) {
  this->ipAddr = ip;
}

Driver* broadcom_BCM5751::getDriver(PeripheralComponentDeviceDescriptor* device, InterruptManager* interruptManager) {
  Driver* ret = 0;

  if(device->vendorId == 0x14E4 && device->deviceId == 0x1677) {
    ret = new broadcom_BCM5751(device, interruptManager);
  }

  return ret;
}
