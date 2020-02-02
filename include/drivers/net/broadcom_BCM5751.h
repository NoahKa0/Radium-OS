#ifndef __MYOS__DRIVERS__BROADCOM_BCM5751_H
#define __MYOS__DRIVERS__BROADCOM_BCM5751_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <drivers/net/ethernet_driver.h>
  #include <hardware/interrupts.h>
  #include <hardware/pci.h>
  #include <hardware/port.h>
  #include <memorymanagement.h>
  #include <timer.h>
  
  namespace sys {
    namespace drivers {
      enum {					/* registers */
        Bmcr		= 0x00,		/* Basic Mode Control */
        Bmsr		= 0x01,		/* Basic Mode Status */
        Phyidr1		= 0x02,		/* PHY Identifier #1 */
        Phyidr2		= 0x03,		/* PHY Identifier #2 */
        Anar		= 0x04,		/* Auto-Negotiation Advertisement */
        Anlpar		= 0x05,		/* AN Link Partner Ability */
        Aner		= 0x06,		/* AN Expansion */
        Annptr		= 0x07,		/* AN Next Page TX */
        Annprr		= 0x08,		/* AN Next Page RX */
        Mscr		= 0x09,		/* MASTER-SLAVE Control */
        Mssr		= 0x0a,		/* MASTER-SLAVE Status */
        Esr		= 0x0f,		/* Extended Status */

        NMiiPhyr	= 32,
        NMiiPhy		= 32,
      };
      enum {
        Ana10HD		= 0x0020,	/* Advertise 10BASE-T */
        Ana10FD		= 0x0040,	/* Advertise 10BASE-T FD */
        AnaTXHD		= 0x0080,	/* Advertise 100BASE-TX */
        AnaTXFD		= 0x0100,	/* Advertise 100BASE-TX FD */
        AnaT4		= 0x0200,	/* Advertise 100BASE-T4 */
        AnaP		= 0x0400,	/* Pause */
        AnaAP		= 0x0800,	/* Asymmetrical Pause */
        AnaRf		= 0x2000,	/* Remote Fault */
        AnaAck		= 0x4000,	/* Acknowledge */
        AnaNp		= 0x8000,	/* Next Page Indication */

	      Mssr1000THD	= 0x0400,	/* Link Partner 1000BASE-T HD able */
	      Mssr1000TFD	= 0x0800,	/* Link Partner 1000BASE-T FD able */
      };

      enum {
        /* configurable constants */
        RxRetRingLen 		= 0x200,
        RxProdRingLen 		= 0x200,
        TxRingLen 		= 0x200,

        Reset 			= 1<<0,
        Enable 			= 1<<1,
        Attn 			= 1<<2,

        Pwrctlstat 		= 0x4C,

        MiscHostCtl 		= 0x68,
        TaggedStatus		= 1<<9,
        IndirAccessEn		= 1<<7,
        EnableClockCtl		= 1<<5,
        PCIStateRegEn		= 1<<4,
        WordSwap		= 1<<3,
        ByteSwap		= 1<<2,
        MaskPCIInt		= 1<<1,
        ClearIntA		= 1<<0,

        Fwmbox			= 0x0b50,	/* magic value exchange */
        Fwmagic			= 0x4b657654,

        Dmarwctl 		= 0x6C,
        DMAWaterMask		= ~(7<<19),
        DMAWaterValue		= 3<<19,

        Memwind		= 0x7C,
        MemwindData		= 0x84,

        TxRCB 			= 0x100,
        RxRetRCB 		= 0x200,

        InterruptMailbox 		= 0x204,

        RxProdBDRingIdx	= 0x26c,
        RxBDRetRingIdx		= 0x284,
        TxBDRingHostIdx	= 0x304,

        MACMode		= 0x400,
        MACPortMask		= ~(1<<3 | 1<<2),
        MACPortGMII		= 1<<3,
        MACPortMII		= 1<<2,
        MACEnable		= 1<<23 | 1<<22 | 1<<21 | 1 << 15 | 1 << 14 | 1<<12 | 1<<11,
        MACHalfDuplex		= 1<<1,

        MACEventStatus		= 0x404,
        MACEventEnable	= 0x408,
        MACAddress		= 0x410,
        RandomBackoff		= 0x438,
        RxMTU			= 0x43C,
        MIComm		= 0x44C,
        MIStatus		= 0x450,
        MIMode			= 0x454,
        RxMACMode		= 0x468,
        TxMACMode		= 0x45C,
        TxMACLengths		= 0x464,
        MACHash		= 0x470,
        RxRules			= 0x480,

        RxRulesConf		= 0x500,
        LowWaterMax		= 0x504,
        LowWaterMaxMask	= ~0xFFFF,
        LowWaterMaxValue	= 2,

        TxDataInitiatorMode	= 0xC00,
        TxInitiatorConf		= 0x0C08,
        TxStats			= 1<<0,
        TxInitiatorMask		= 0x0C0C,

        TxDataCompletionMode	= 0x1000,
        TxBDSelectorMode	= 0x1400,
        TxBDInitiatorMode	= 0x1800,
        TxBDCompletionMode	= 0x1C00,

        RxListPlacementMode	= 0x2000,
        RxListPlacement		= 0x2010,
        RxListPlacementConf	= 0x2014,
        RxStats			= 1<<0,
        RxListPlacementMask	= 0x2018,

        RxDataBDInitiatorMode	= 0x2400,
        RxBDHostAddr		= 0x2450,
        RxBDFlags		= 0x2458,
        RxBDNIC		= 0x245C,
        RxDataCompletionMode	= 0x2800,
        RxBDInitiatorMode	= 0x2C00,
        RxBDRepl		= 0x2C18,

        RxBDCompletionMode	= 0x3000,
        HostCoalMode		= 0x3C00,
        HostCoalRxTicks		= 0x3C08,
        HostCoalTxTicks		= 0x3C0C,
        RxMaxCoalFrames	= 0x3C10,
        TxMaxCoalFrames	= 0x3C14,
        RxMaxCoalFramesInt	= 0x3C20,
        TxMaxCoalFramesInt	= 0x3C24,
        StatusBlockHostAddr	= 0x3C38,
        FlowAttention		= 0x3C48,

        MemArbiterMode		= 0x4000,

        BufferManMode		= 0x4400,

        MBUFLowWater		= 0x4414,
        MBUFHighWater		= 0x4418,

        ReadDMAMode		= 0x4800,
        ReadDMAStatus		= 0x4804,
        WriteDMAMode		= 0x4C00,
        WriteDMAStatus		= 0x4C04,

        RISCState		= 0x5004,
        FTQReset		= 0x5C00,
        MSIMode		= 0x6000,

        ModeControl		= 0x6800,
        ByteWordSwap		= 1<<4 | 1<<5 | 1<<2, // | 1<<1,
        HostStackUp		= 1<<16,
        HostTxBDs		= 1<<17,
        InterruptOnMAC		= 1<<26,

        MiscConf		= 0x6804,
        CoreClockBlocksReset	= 1<<0,
        GPHYPwrdnOverride	= 1<<26,
        DisableGRCRstOnPpcie	= 1<<29,
        TimerMask		= ~0xFF,
        TimerValue		= 65<<1,
        MiscLocalControl		= 0x6808,
        InterruptOnAttn		= 1<<3,
        AutoSEEPROM		= 1<<24,

        SwArbit			= 0x7020,
        SwArbitSet1		= 1<<1,
        SwArbitWon1		= 1<<9,
        Pcitlplpl			= 0x7C00,	/* "lower 1k of the pcie pl regs" ?? */

        PhyAuxControl		= 0x18,
        PhyIntStatus		= 0x1A,
        PhyIntMask		= 0x1B,

        Updated			= 1<<0,
        LinkStateChange		= 1<<1,
        Error			= 1<<2,

        PacketEnd		= 1<<2,
        FrameError		= 1<<10,
      };
      
      struct Block {
        Block* next;

        common::uint8_t* rp;
        common::uint8_t* wp;
        common::uint8_t* lim;

        common::uint8_t*	base; // Used to be array with unspecified bounds, but compiler started complaining of invalid use...
      }  __attribute__((packed));
      
      struct BCM5751_Ctlr {
        BCM5751_Ctlr* link;
        hardware::PeripheralComponentDeviceDescriptor* pciDevice;
        common::uint32_t* nic;
        common::uint32_t* status;
        
        common::uint32_t* recvret;
        common::uint32_t* recvprod;
        common::uint32_t* sendr;
        
        common::uint32_t port;
        common::uint32_t recvreti, recvprodi, sendri, sendcleani;
        Block** sends;
        Block** rxs;
        int active, duplex;
      };
      
      struct Etherpacket { // NOTICE We already have a header structure in /net/etherframe. I should use that one instead.
        common::uint8_t d[6];
        common::uint8_t s[6];
        common::uint8_t type[2];
        common::int8_t data[1500];
      }  __attribute__((packed));
      
      class broadcom_BCM5751 : public EthernetDriver, public hardware::InterruptHandler {
      private:
        sys::hardware::PeripheralComponentDeviceDescriptor* device;
        common::uint64_t macAddress; // Original stored in EDev, but i don't have EDev.
        BCM5751_Ctlr ctlr;

        Block** sendBuffer;
        common::uint32_t sendRead;
        common::uint32_t sendWrite;
        
        // Allocb, memset, paddr and struct block where actually not part of this driver, but i didn't have them.
        Block* allocb(common::uint32_t size);
        
        void bcmtransclean();
        
        static common::uint64_t dummyread(common::uint64_t n);
        static void memset(void* dst, common::int32_t v, common::int32_t n);
        static common::uint64_t paddr(common::uint64_t a);
        
        static common::int32_t miir(common::uint32_t* nic, common::int32_t ra);
        static common::int32_t miiw(common::uint32_t* nic, common::int32_t ra, common::int32_t value);
        
        common::uint32_t* currentrecvret();
        void consumerecvret();
        
        void checklink();
        
        common::int32_t replenish(Block* bp = 0); // The original driver doesn't allow you to pass a block, but if i do the same thing it will reacllocate the same block. I think reusing them is better.
        
        common::int32_t mbps;
        common::int32_t link;
        common::uint32_t ipAddr;
      public:
        broadcom_BCM5751(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
        ~broadcom_BCM5751();
        
        virtual void activate();
        virtual int reset();
        
        virtual common::uint32_t handleInterrupt(common::uint32_t esp);
        
        virtual void send(common::uint8_t* buffer, int size);
        void transmit();

        virtual void receive();
        virtual common::uint64_t getMacAddress();
        
        virtual common::uint32_t getIpAddress();
        virtual void setIpAddress(common::uint32_t ip);
      };
    }
  }
    
#endif
