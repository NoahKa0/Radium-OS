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
  b = (Block*) MemoryManager::activeMemoryManager->malloc((sizeof(Block)+size+(64))*2);
  
  if(b == 0) {
    printf("broadcom_BCM5751-allocb->malloc failed!\nPANIC!\n");
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
  
  b->lim = (uint8_t*) b + (sizeof(Block)+size+(64));

  // leave space for added headers
  b->rp = b->lim - size;
  if(b->rp < b->base) {
    printf("broadcom_BCM5751-allocb rp < base!\nPANIC!\n");
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
		this->ctlr.sendcleani = (this->ctlr.sendcleani + 1) & (SendRingLen - 1);
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
  uint64_t* next;
  uint32_t incr;
  uint32_t idx;
  
  printf("+");

  idx = this->ctlr.recvprodi;
  incr = (this->ctlr.recvprodi + 1) & (RecvProdRingLen - 1);
  if(incr == (this->ctlr.status[2] >> 16)) return -1;
  if(this->ctlr.recvs[idx] != 0) return -1;
  if(bp == 0) {
    bp = this->allocb(Rbsz);
  }
  if(bp == 0) {
    printf("bcm: out of memory for receive buffers\n");
    return -1;
  }
  this->ctlr.recvs[idx] = bp;
  next = this->ctlr.recvprod + this->ctlr.recvprodi * 8;
  memset(next, 0, 32);
  next[1] = this->paddr((uint32_t) bp->rp);
  next[2] = Rbsz;
  next[7] = (uint32_t) bp;
  csr32(this->ctlr.nic, RecvProdBDRingIndex) = this->ctlr.recvprodi = incr;
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

uint64_t* broadcom_BCM5751::currentrecvret() {
	if(this->ctlr.recvreti == (this->ctlr.status[4] & 0xFFFF)) return 0;
	return this->ctlr.recvret + this->ctlr.recvreti * 8;
}

void broadcom_BCM5751::consumerecvret() {
	csr32(this->ctlr.nic, RecvBDRetRingIndex) = this->ctlr.recvreti = (this->ctlr.recvreti + 1) & (RecvRetRingLen - 1);
}

void broadcom_BCM5751::checklink() {
  uint32_t i;
  uint32_t* nic = this->ctlr.nic;
  
  miir(nic, PhyStatus); /* dummy read necessary */
  if(!(miir(nic, PhyStatus) & PhyLinkStatus)) {
    this->link = 0;
    this->mbps = 1000;
    this->ctlr.duplex = 1;
    printf("bcm: no link\n");
    goto out;
  }
  this->link = 1;
  while((miir(nic, PhyStatus) & PhyAutoNegComplete) == 0);
  i = miir(nic, PhyGbitStatus);
  if(i & (Phy1000FD | Phy1000HD)) {
    this->mbps = 1000;
    this->ctlr.duplex = (i & Phy1000FD) != 0;
  } else if(i = miir(nic, PhyPartnerStatus), i & (Phy100FD | Phy100HD)) {
    this->mbps = 100;
    this->ctlr.duplex = (i & Phy100FD) != 0;
  } else if(i & (Phy10FD | Phy10HD)) {
    this->mbps = 10;
    this->ctlr.duplex = (i & Phy10FD) != 0;
  } else {
    this->link = 0;
    this->mbps = 1000;
    this->ctlr.duplex = 1;
    printf("bcm: link partner supports neither 10/100/1000 Mbps\n"); 
    goto out;
  }
  printf("broadcom_BCM5751: ");
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
    printf("BCM5751 bar is not set to memory mapped!\nPANIC!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }

  // Enable MAC memory space decode and bus mastering.
  uint32_t pciCommandRead = this->device->read(0x04);
  pciCommandRead |= 0x06;
  this->device->write(0x04, pciCommandRead);
  
  this->macAddress = 0;
  this->mbps = 0;
  this->link = 0;
  
  this->ctlr.nic = (uint32_t*) device->addressBase;
  this->ctlr.port = (uint32_t) device->portBase;
  
  this->ctlr.status = (uint32_t*) this->allocb(20+16);
  this->ctlr.recvprod = (uint64_t*) this->allocb(32 * RecvProdRingLen + 16);
  this->ctlr.recvret = (uint64_t*) this->allocb(32 * RecvRetRingLen + 16);
  this->ctlr.sendr = (uint64_t*) this->allocb(16 * SendRingLen + 16);
  
  this->ctlr.sends = (Block**) MemoryManager::activeMemoryManager->malloc(sizeof(this->ctlr.sends[0]) * SendRingLen);
  this->ctlr.recvs = (Block**) MemoryManager::activeMemoryManager->malloc(sizeof(this->ctlr.recvs[0]) * RecvProdRingLen);
  
  setSelectedEthernetDriver(this); // Make this instance accessable in kernel.cpp
}

broadcom_BCM5751::~broadcom_BCM5751() {}

void broadcom_BCM5751::activate() {
  uint64_t i, j;
  uint32_t* nic = this->ctlr.nic;
  uint32_t* mem = nic + 0x2000;
  
  printf("BCM Starting at ");
  printHex32((uint32_t) nic);
  printf("\n");
  
  csr32(nic, MiscHostCtl) |= MaskPCIInt | ClearIntA;
  csr32(nic, SwArbitration) |= SwArbitSet1;
  
  printf("While 1 ");
  printHex32((csr32(nic, SwArbitration)));
  printf("  ");
  printHex32(SwArbitWon1);
  while((csr32(nic, SwArbitration) & SwArbitWon1) == 0) {
    SystemTimer::sleep(40);
  }
  printf("'n");
  
  csr32(nic, MemArbiterMode) |= Enable;
  csr32(nic, MiscHostCtl) |= IndirectAccessEnable | EnablePCIStateRegister | EnableClockControlRegister;
  csr32(nic, MiscHostCtl) = (csr32(nic, MiscHostCtl) & ~(1<<2|1<<3)) | 1<<3;
	csr32(nic, ModeControl) |= ByteWordSwap;
  csr32(nic, MemoryWindow) = 0;
  csr32(mem, 0xB50) = 0x4B657654; // magic number
  
  csr32(nic, MiscConfiguration) |= GPHYPowerDownOverride | DisableGRCResetOnPCIE;
  csr32(nic, MiscConfiguration) |= CoreClockBlocksReset;
  
  SystemTimer::sleep(150); // I should wait 100 ms, but the timer isn't that accurate, so wait slightly more.
  
  // Enable MAC memory space decode and bus mastering.
  uint32_t pciCommandRead = this->device->read(0x04);
  pciCommandRead |= 0x06; // 0x04 | 0x02
  this->device->write(0x04, pciCommandRead);
  
  printf(" ");
  printHex32(pciCommandRead);
  printf(" ");
  printHex32(this->device->read(0x04));
  printf("\n");
  
  csr32(nic, MiscHostCtl) |= MaskPCIInt;
  csr32(nic, MemArbiterMode) |= Enable;
  csr32(nic, MiscHostCtl) |= IndirectAccessEnable | EnablePCIStateRegister | EnableClockControlRegister | TaggedStatus;
  csr32(nic, ModeControl) |= ByteWordSwap;
  csr32(nic, MACMode) = (csr32(nic, MACMode) & MACPortMask) | MACPortGMII;
  
  SystemTimer::sleep(100); // Original driver sleeps for 40 ms, but the timer isn't that accurate.
  
  printf("While 2\n");
  while(csr32(mem, 0xB50) != 0xB49A89AB) {
    SystemTimer::sleep(100);
  }
  
  csr32(nic, TLPControl) |= (1<<25) | (1<<29);
  printf("Memset");
  this->memset(this->ctlr.status, 0, 20);
  printf(" eoms\n");
  // --
  csr32(nic, DMARWControl) = (csr32(nic, DMARWControl) & DMAWatermarkMask) | DMAWatermarkValue;
  csr32(nic, ModeControl) |= HostSendBDs | HostStackUp | InterruptOnMAC;
  csr32(nic, MiscConfiguration) = (csr32(nic, MiscConfiguration) & TimerMask) | TimerValue;
  csr32(nic, MBUFLowWatermark) = 0x20;
  csr32(nic, MBUFHighWatermark) = 0x60;
  csr32(nic, LowWatermarkMaximum) = (csr32(nic, LowWatermarkMaximum) & LowWatermarkMaxMask) | LowWatermarkMaxValue;
  csr32(nic, BufferManMode) |= Enable | Attn;
  while((csr32(nic, BufferManMode) & Enable) == 0);
  csr32(nic, FTQReset) = -1;
  csr32(nic, FTQReset) = 0;
  while(csr32(nic, FTQReset));
  csr32(nic, ReceiveBDHostAddr) = 0;
  csr32(nic, ReceiveBDHostAddr + 4) = paddr((uint64_t) this->ctlr.recvprod);
  csr32(nic, ReceiveBDFlags) = RecvProdRingLen << 16;
  csr32(nic, ReceiveBDNIC) = 0x6000;
  csr32(nic, ReceiveBDRepl) = 25;
  csr32(nic, SendBDRingHostIndex) = 0;
  csr32(nic, SendBDRingHostIndex+4) = 0;
  csr32(mem, SendRCB) = 0;
  csr32(mem, SendRCB + 4) = paddr((uint64_t) this->ctlr.sendr);
  csr32(mem, SendRCB + 8) = SendRingLen << 16;
  csr32(mem, SendRCB + 12) = 0x4000;
  
  for(i=1;i<4;i++)
    csr32(mem, RecvRetRCB + i * 0x10 + 8) = 2;
  csr32(mem, RecvRetRCB) = 0;
  csr32(mem, RecvRetRCB + 4) = paddr((uint64_t) this->ctlr.recvret);
  csr32(mem, RecvRetRCB + 8) = RecvRetRingLen << 16;
  csr32(nic, RecvProdBDRingIndex) = 0;
  csr32(nic, RecvProdBDRingIndex+4) = 0;
  
  asm("sti");
  SystemTimer::sleep(100);
  asm("cli");
  
  i = csr32(nic, 0x410); // I don't know what this does, i also don't know why it is stored with the address.
  // j = edev->ea[0] = i >> 8;
  // j += edev->ea[1] = i;
  
  i = csr32(nic, MACAddress + 4);
  this->macAddress = i;
  // NOTICE original, remove later.
  // j += edev->ea[2] = i >> 24;
  // j += edev->ea[3] = i >> 16;
  // j += edev->ea[4] = i >> 8;
  // j += edev->ea[5] = i;
  
  csr32(nic, EthernetRandomBackoff) = j & 0x3FF;
  csr32(nic, ReceiveMTU) = Rbsz;
  csr32(nic, TransmitMACLengths) = 0x2620;
  csr32(nic, ReceiveListPlacement) = 1<<3;
  csr32(nic, ReceiveListPlacementMask) = 0xFFFFFF;
  csr32(nic, ReceiveListPlacementConfiguration) |= ReceiveStats;
  csr32(nic, SendInitiatorMask) = 0xFFFFFF;
  csr32(nic, SendInitiatorConfiguration) |= SendStats;
  csr32(nic, HostCoalescingMode) = 0;
  while(csr32(nic, HostCoalescingMode) != 0);
  csr32(nic, HostCoalescingRecvTicks) = 150;
  csr32(nic, HostCoalescingSendTicks) = 150;
  csr32(nic, RecvMaxCoalescedFrames) = 10;
  csr32(nic, SendMaxCoalescedFrames) = 10;
  csr32(nic, RecvMaxCoalescedFramesInt) = 0;
  csr32(nic, SendMaxCoalescedFramesInt) = 0;
  csr32(nic, StatusBlockHostAddr) = 0;
  csr32(nic, StatusBlockHostAddr + 4) = paddr((uint64_t) this->ctlr.status);
  csr32(nic, HostCoalescingMode) |= Enable;
  csr32(nic, ReceiveBDCompletionMode) |= Enable | Attn;
  csr32(nic, ReceiveListPlacementMode) |= Enable;
  csr32(nic, MACMode) |= MACEnable;
  csr32(nic, MiscLocalControl) |= InterruptOnAttn | AutoSEEPROM;
  csr32(nic, InterruptMailbox) = 0;
  csr32(nic, WriteDMAMode) |= 0x200003fe;
  csr32(nic, ReadDMAMode) |= 0x3fe;
  csr32(nic, ReceiveDataCompletionMode) |= Enable | Attn;
  csr32(nic, SendDataCompletionMode) |= Enable;
  csr32(nic, SendBDCompletionMode) |= Enable | Attn;
  csr32(nic, ReceiveBDInitiatorMode) |= Enable | Attn;
  csr32(nic, ReceiveDataBDInitiatorMode) |= Enable | (1<<4);
  csr32(nic, SendDataInitiatorMode) |= Enable;
  csr32(nic, SendBDInitiatorMode) |= Enable | Attn;
  csr32(nic, SendBDSelectorMode) |= Enable | Attn;
  this->ctlr.recvprodi = 0;
  
  while(this->replenish() >= 0);
  
  csr32(nic, TransmitMACMode) |= Enable;
  csr32(nic, ReceiveMACMode) |= Enable;
  csr32(nic, PowerControlStatus) &= ~3;
  csr32(nic, MIStatus) |= 1<<0;
  csr32(nic, MACEventEnable) = 0;
  csr32(nic, MACEventStatus) |= (1<<12);
  csr32(nic, MIMode) = 0xC0000;
  
  asm("sti");
  SystemTimer::sleep(100);
  asm("cli");
  
  this->miiw(nic, PhyControl, 1<<15);
  while(this->miir(nic, PhyControl) & (1<<15));
  this->miiw(nic, PhyAuxControl, 2);
  this->miir(nic, PhyIntStatus);
  this->miir(nic, PhyIntStatus);
  this->miiw(nic, PhyIntMask, ~(1<<1));
  
  checklink();
  
  csr32(nic, MACEventEnable) |= 1<<12;
  csr32(nic, MACHash) = -1;
  csr32(nic, MACHash+4) = -1;
  csr32(nic, MACHash+8) = -1;
  csr32(nic, MACHash+12) = -1;
  for(i = 0; i < 8; i++) csr32(nic, ReceiveRules + 8 * i) = 0;
  csr32(nic, ReceiveRulesConfiguration) = 1 << 3;
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
        printf("bcm: fatal error!\nPANIC!");
        while(true) {
          asm("cli");
          asm("hlt");
        }
      }
      csr32(nic, FlowAttention) = 0;
    }
    csr32(nic, MACEventStatus) = 0; /* worth ignoring */
    if(csr32(nic, ReadDMAStatus) || csr32(nic, WriteDMAStatus)) {
      printf("DMA!    ");
      csr32(nic, ReadDMAStatus) = 0;
      csr32(nic, WriteDMAStatus) = 0;
    }
    if(csr32(nic, RISCState)) {
      if(csr32(nic, RISCState) & 0x78000403) {
        printf("bcm: RISC halted!\nPANIC!");
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
	csr32(nic, InterruptMailbox) = tag << 24;
  
  return esp;
}

void broadcom_BCM5751::send(common::uint8_t* buffer, int size) {
  if(size > sizeof(Etherpacket)) {
    size = sizeof(Etherpacket);
  }

  this->bcmtransclean();
  
  uint64_t incr = 0;
  uint64_t* next = 0;
  Block* bp = 0;

  incr = (this->ctlr.sendri + 1) & (SendRingLen - 1);
  if(incr == (this->ctlr.status[4] >> 16)) {
    printf("bcm: send queue full\n");
    return;
  }

  if(incr == this->ctlr.sendcleani) {
    if(incr == this->ctlr.sendcleani)
      return;
  }
  
  // The original driver fetches a block from a que, but we don't have that, so setup a new one.
  bp = this->allocb(Rbsz);
  if(bp == 0) return;
  bp->wp = bp->rp + size;
  uint8_t* blockData = bp->rp;
  for(int i = 0; i < size; i++) {
    blockData[i] = buffer[i];
  }

  next = this->ctlr.sendr + this->ctlr.sendri * 4;
  next[0] = 0;
  next[1] = this->paddr((uint32_t) bp->rp);
  next[2] = (BLEN(bp) << 16) | PacketEnd;
  next[3] = 0;
  this->ctlr.sends[this->ctlr.sendri] = bp;
  csr32(this->ctlr.nic, SendBDRingHostIndex) = this->ctlr.sendri = incr;
}

void broadcom_BCM5751::receive() {
  Block* bp;
  uint64_t* pkt;
  uint32_t len;
  uint64_t idx;
  
  while(pkt = currentrecvret()) {
		idx = pkt[7] & (RecvProdRingLen - 1);
		bp = this->ctlr.recvs[idx];

    if(bp == 0) {
			printf("bcm: nil block -- shouldn't happen\n");
			break;
		}

    this->ctlr.recvs[idx] = 0;
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
  // TODO
  return 0;
}

void broadcom_BCM5751::setIpAddress(uint32_t ip) {
  // TODO
}
