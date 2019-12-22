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
  if((b = (Block*) MemoryManager::activeMemoryManager->malloc(sizeof(Block)+size+(64))) == 0) {
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
  BCM5751_Ctlr ctlr = this->ctlr;
  while(ctlr.sendcleani != (ctlr.status[4] >> 16)) {
		MemoryManager::activeMemoryManager->free(ctlr.sends[ctlr.sendcleani]);
		ctlr.sends[ctlr.sendcleani] = 0;
		ctlr.sendcleani = (ctlr.sendcleani + 1) & (SendRingLen - 1);
	}
}

int32_t broadcom_BCM5751::miir(uint64_t* nic, int32_t ra) {
  while(csr32(nic, MIComm) & (1<<29));
  csr32(nic, MIComm) = (ra << 16) | (1 << 21) | (1 << 27) | (1 << 29);
  while(csr32(nic, MIComm) & (1<<29));
  if(csr32(nic, MIComm) & (1<<28)) return -1;
  return csr32(nic, MIComm) & 0xFFFF;
}

int32_t broadcom_BCM5751::miiw(uint64_t* nic, int32_t ra, int32_t value) {
  while(csr32(nic, MIComm) & (1<<29));
  csr32(nic, MIComm) = (value & 0xFFFF) | (ra << 16) | (1 << 21) | (1 << 27) | (1 << 29);
  while(csr32(nic, MIComm) & (1<<29));
  return 0;
}

common::int32_t broadcom_BCM5751::replenish(Block* bp) {
  BCM5751_Ctlr ctlr = this->ctlr;
  uint64_t* next;
  uint64_t incr;

  incr = (ctlr.recvprodi + 1) & (RecvProdRingLen - 1);
  if(incr == (ctlr.status[2] >> 16)) return -1;
  if(bp == 0) {
    bp = this->allocb(Rbsz);
  }
  if(bp == 0) {
    printf("bcm: out of memory for receive buffers\n");
    return -1;
  }
  next = ctlr.recvprod + ctlr.recvprodi * 8;
  memset(next, 0, 32);
  next[1] = this->paddr((uint64_t) bp->rp);
  next[2] = Rbsz;
  next[7] = (uint64_t) bp;
  csr32(ctlr.nic, RecvProdBDRingIndex) = ctlr.recvprodi = incr;
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
  uint64_t i;
  
  BCM5751_Ctlr ctlr = this->ctlr;
  uint64_t* nic = ctlr.nic;
  
  miir(nic, PhyStatus); /* dummy read necessary */
  if(!(miir(nic, PhyStatus) & PhyLinkStatus)) {
    this->link = 0;
    this->mbps = 1000;
    ctlr.duplex = 1;
    printf("bcm: no link\n");
    goto out;
  }
  this->link = 1;
  while((miir(nic, PhyStatus) & PhyAutoNegComplete) == 0);
  i = miir(nic, PhyGbitStatus);
  if(i & (Phy1000FD | Phy1000HD)) {
    this->mbps = 1000;
    ctlr.duplex = (i & Phy1000FD) != 0;
  } else if(i = miir(nic, PhyPartnerStatus), i & (Phy100FD | Phy100HD)) {
    this->mbps = 100;
    ctlr.duplex = (i & Phy100FD) != 0;
  } else if(i & (Phy10FD | Phy10HD)) {
    this->mbps = 10;
    ctlr.duplex = (i & Phy10FD) != 0;
  } else {
    this->link = 0;
    this->mbps = 1000;
    ctlr.duplex = 1;
    printf("bcm: link partner supports neither 10/100/1000 Mbps\n"); 
    goto out;
  }
  printf("broadcom_BCM5751: ");
  printHex32(this->mbps);
  printf("Mbps ");
  if(ctlr.duplex) {
    printf("full ");
  } else {
    printf("half ");
  }
  printf("duplex\n");
out:
  if(ctlr.duplex) csr32(nic, MACMode) &= ~MACHalfDuplex;
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
  if(device->memoryMapped != true) {
    printf("BCM5751 bar is not set to memory mapped!\nPANIC!");
    while(true) {
      asm("cli");
      asm("hlt");
    }
  }
  
  this->macAddress = 0;
  this->mbps = 0;
  this->link = 0;
  
  BCM5751_Ctlr ctlr = this->ctlr;
  ctlr.nic = (uint64_t*) (device->portBase & ~0x0F);
  ctlr.port = (uint64_t) (device->portBase & ~0x0F);
  
  ctlr.status = (uint64_t*) this->allocb(20+16);
  ctlr.recvprod = (uint64_t*) this->allocb(32 * RecvProdRingLen + 16);
  ctlr.recvret = (uint64_t*) this->allocb(32 * RecvRetRingLen + 16);
  ctlr.sendr = (uint64_t*) this->allocb(16 * SendRingLen + 16);
  
  // NOTICE It seems weird to me that sizeof(Block) is used here, since it's an array of Block*.
  // I think this might be a mistake, but since it allocates too much, nothing horrible happens. Just leave it as it is for now.
  ctlr.sends = (Block**) MemoryManager::activeMemoryManager->malloc(sizeof(Block) * SendRingLen);
  
  setSelectedEthernetDriver(this); // Make this instance accessable in kernel.cpp
}

broadcom_BCM5751::~broadcom_BCM5751() {}

void broadcom_BCM5751::activate() {
  asm("cli");
  
  uint64_t i, j;
  uint64_t* nic = this->ctlr.nic;
  uint64_t* mem = nic + 0x8000;
  
  csr32(nic, MiscHostCtl) |= MaskPCIInt | ClearIntA;
  csr32(nic, SwArbitration) |= SwArbitSet1;
  while((csr32(nic, SwArbitration) & SwArbitWon1) == 0);
  
  csr32(nic, MemArbiterMode) |= Enable;
  csr32(nic, MiscHostCtl) |= IndirectAccessEnable | EnablePCIStateRegister | EnableClockControlRegister;
  csr32(nic, MemoryWindow) = 0;
  csr32(mem, 0xB50) = 0x4B657654; // magic number
  
  csr32(nic, MiscConfiguration) |= GPHYPowerDownOverride | DisableGRCResetOnPCIE;
  csr32(nic, MiscConfiguration) |= CoreClockBlocksReset;
  
  asm("sti");
  SystemTimer::sleep(150); // I should wait 100 ms, but the timer isn't that accurate, so wait slightly more.
  asm("cli");
  // NOTICE I can't find pcr in pdev, so i ignore it for now, and hope for the best.
  // ctlr->pdev->pcr |= 1<<1; // This is the original line of code from etherbcm.c
  
  // NOTICE pcisetbme(ctlr->pdev); // I don't know exactly what this does. It doesn't seem important, but i might be wrong.
  csr32(nic, MiscHostCtl) |= MaskPCIInt;
  csr32(nic, MemArbiterMode) |= Enable;
  csr32(nic, MiscHostCtl) |= IndirectAccessEnable | EnablePCIStateRegister | EnableClockControlRegister | TaggedStatus;
  csr32(nic, ModeControl) |= ByteWordSwap;
  csr32(nic, MACMode) = (csr32(nic, MACMode) & MACPortMask) | MACPortGMII;
  
  asm("sti");
  SystemTimer::sleep(100); // Original driver sleeps for 40 ms, but the timer isn't that accurate.
  asm("cli");
  
  while(csr32(mem, 0xB50) != 0xB49A89AB);
  csr32(nic, TLPControl) |= (1<<25) | (1<<29);
  this->memset(ctlr.status, 0, 20);
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
  csr32(nic, ReceiveBDHostAddr + 4) = paddr((uint64_t) ctlr.recvprod);
  csr32(nic, ReceiveBDFlags) = RecvProdRingLen << 16;
  csr32(nic, ReceiveBDNIC) = 0x6000;
  csr32(nic, ReceiveBDRepl) = 25;
  csr32(nic, SendBDRingHostIndex) = 0;
  csr32(nic, SendBDRingHostIndex+4) = 0;
  csr32(mem, SendRCB) = 0;
  csr32(mem, SendRCB + 4) = paddr((uint64_t) ctlr.sendr);
  csr32(mem, SendRCB + 8) = SendRingLen << 16;
  csr32(mem, SendRCB + 12) = 0x4000;
  
  for(i=1;i<4;i++)
    csr32(mem, RecvRetRCB + i * 0x10 + 8) = 2;
  csr32(mem, RecvRetRCB) = 0;
  csr32(mem, RecvRetRCB + 4) = paddr((uint64_t) ctlr.recvret);
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
  csr32(nic, StatusBlockHostAddr + 4) = paddr((uint64_t) ctlr.status);
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
  ctlr.recvprodi = 0;
  
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
  BCM5751_Ctlr ctlr = this->ctlr;
  uint64_t* nic = ctlr.nic;
  uint64_t status = 0;
  uint64_t tag = 0;
  
	broadcom_BCM5751::dummyread(csr32(nic, InterruptMailbox));
	csr32(nic, InterruptMailbox) = 1;
	status = ctlr.status[0];
	tag = ctlr.status[1];
	ctlr.status[0] = 0;
  
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
      printf("bcm: DMA error\n");
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
  
  BCM5751_Ctlr ctlr = this->ctlr;
  uint64_t incr = 0;
  uint64_t* next = 0;
  Block* bp = 0;
  
  while(true) {
    incr = (ctlr.sendri + 1) & (SendRingLen - 1);
		if(incr == (ctlr.status[4] >> 16)) {
			printf("bcm: send queue full\n");
			break;
		}
		if(incr == ctlr.sendcleani) {
			this->bcmtransclean();
			if(incr == ctlr.sendcleani)
				break;
		}
		
		// The original driver fetches a block from a que, but we don't have that, so setup a new one.
		bp = this->allocb(Rbsz);
    bp->wp = bp->rp + size;
    uint8_t* blockData = bp->rp;
    for(int i = 0; i < size; i++) {
      blockData[i] = buffer[i];
    }
    
		if(bp == 0) break;
		next = ctlr.sendr + ctlr.sendri * 4;
		next[0] = 0;
		next[1] = this->paddr((uint64_t) bp->rp);
		next[2] = (BLEN(bp) << 16) | PacketEnd;
		next[3] = 0;
		ctlr.sends[ctlr.sendri] = bp;
		csr32(ctlr.nic, SendBDRingHostIndex) = ctlr.sendri = incr;
  }
}

void broadcom_BCM5751::receive() {
  Block* bp;
  uint64_t* pkt;
  uint32_t len;
  
  while(pkt = currentrecvret()) {
    bp = (Block*) pkt[7];
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
