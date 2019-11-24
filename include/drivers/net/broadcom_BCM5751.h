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
      enum {
        RecvRetRingLen = 0x200,
        RecvProdRingLen = 0x200,
        SendRingLen = 0x200,
      };

      enum {
        Reset = 1<<0,
        Enable = 1<<1,
        Attn = 1<<2,
        
        PowerControlStatus = 0x4C,

        MiscHostCtl = 0x68,
        ClearIntA = 1<<0,
        MaskPCIInt = 1<<1,
        IndirectAccessEnable = 1<<7,
        EnablePCIStateRegister = 1<<4,
        EnableClockControlRegister = 1<<5,
        TaggedStatus = 1<<9,
        
        DMARWControl = 0x6C,
        DMAWatermarkMask = ~(7<<19),
        DMAWatermarkValue = 3<<19,

        MemoryWindow = 0x7C,
        MemoryWindowData = 0x84,
        
        SendRCB = 0x100,
        RecvRetRCB = 0x200,
        
        InterruptMailbox = 0x204,
        
        RecvProdBDRingIndex = 0x26c,
        RecvBDRetRingIndex = 0x284,
        SendBDRingHostIndex = 0x304,
        
        MACMode = 0x400,
        MACPortMask = ~((1<<3)|(1<<2)),
        MACPortGMII = 1<<3,
        MACPortMII = 1<<2,
        MACEnable = (1<<23) | (1<<22) | (1<<21) | (1 << 15) | (1 << 14) | (1<<12) | (1<<11),
        MACHalfDuplex = 1<<1,
        
        MACEventStatus = 0x404,
        MACEventEnable = 0x408,
        MACAddress = 0x410,
        EthernetRandomBackoff = 0x438,
        ReceiveMTU = 0x43C,
        MIComm = 0x44C,
        MIStatus = 0x450,
        MIMode = 0x454,
        ReceiveMACMode = 0x468,
        TransmitMACMode = 0x45C,
        TransmitMACLengths = 0x464,
        MACHash = 0x470,
        ReceiveRules = 0x480,
        
        ReceiveRulesConfiguration = 0x500,
        LowWatermarkMaximum = 0x504,
        LowWatermarkMaxMask = ~0xFFFF,
        LowWatermarkMaxValue = 2,

        SendDataInitiatorMode = 0xC00,
        SendInitiatorConfiguration = 0x0C08,
        SendStats = 1<<0,
        SendInitiatorMask = 0x0C0C,
        
        SendDataCompletionMode = 0x1000,
        SendBDSelectorMode = 0x1400,
        SendBDInitiatorMode = 0x1800,
        SendBDCompletionMode = 0x1C00,
        
        ReceiveListPlacementMode = 0x2000,
        ReceiveListPlacement = 0x2010,
        ReceiveListPlacementConfiguration = 0x2014,
        ReceiveStats = 1<<0,
        ReceiveListPlacementMask = 0x2018,
        
        ReceiveDataBDInitiatorMode = 0x2400,
        ReceiveBDHostAddr = 0x2450,
        ReceiveBDFlags = 0x2458,
        ReceiveBDNIC = 0x245C,
        ReceiveDataCompletionMode = 0x2800,
        ReceiveBDInitiatorMode = 0x2C00,
        ReceiveBDRepl = 0x2C18,
        
        ReceiveBDCompletionMode = 0x3000,
        HostCoalescingMode = 0x3C00,
        HostCoalescingRecvTicks = 0x3C08,
        HostCoalescingSendTicks = 0x3C0C,
        RecvMaxCoalescedFrames = 0x3C10,
        SendMaxCoalescedFrames = 0x3C14,
        RecvMaxCoalescedFramesInt = 0x3C20,
        SendMaxCoalescedFramesInt = 0x3C24,
        StatusBlockHostAddr = 0x3C38,
        FlowAttention = 0x3C48,

        MemArbiterMode = 0x4000,
        
        BufferManMode = 0x4400,
        
        MBUFLowWatermark = 0x4414,
        MBUFHighWatermark = 0x4418,
        
        ReadDMAMode = 0x4800,
        ReadDMAStatus = 0x4804,
        WriteDMAMode = 0x4C00,
        WriteDMAStatus = 0x4C04,
        
        RISCState = 0x5004,
        FTQReset = 0x5C00,
        MSIMode = 0x6000,
        
        ModeControl = 0x6800,
        ByteWordSwap = (1<<4)|(1<<5)|(1<<2),//|(1<<1),
        HostStackUp = 1<<16,
        HostSendBDs = 1<<17,
        InterruptOnMAC = 1<<26,
        
        MiscConfiguration = 0x6804,
        CoreClockBlocksReset = 1<<0,
        GPHYPowerDownOverride = 1<<26,
        DisableGRCResetOnPCIE = 1<<29,
        TimerMask = ~0xFF,
        TimerValue = 65<<1,
        MiscLocalControl = 0x6808,
        InterruptOnAttn = 1<<3,
        AutoSEEPROM = 1<<24,
        
        SwArbitration = 0x7020,
        SwArbitSet1 = 1<<1,
        SwArbitWon1 = 1<<9,
        TLPControl = 0x7C00,
        
        PhyControl = 0x00,
        PhyStatus = 0x01,
        PhyLinkStatus = 1<<2,
        PhyAutoNegComplete = 1<<5,
        PhyPartnerStatus = 0x05,
        Phy100FD = 1<<8,
        Phy100HD = 1<<7,
        Phy10FD = 1<<6,
        Phy10HD = 1<<5,
        PhyGbitStatus = 0x0A,
        Phy1000FD = 1<<12,
        Phy1000HD = 1<<11,
        PhyAuxControl = 0x18,
        PhyIntStatus = 0x1A,
        PhyIntMask = 0x1B,
        
        Updated = 1<<0,
        LinkStateChange = 1<<1,
        Error = 1<<2,
        
        PacketEnd = 1<<2,
        FrameError = 1<<10,
      };
      
      struct BCM5751_Ctlr {
        BCM5751_Ctlr* link;
        hardware::PeripheralComponentDeviceDescriptor* pciDevice;
        common::uint64_t* nic;
        common::uint64_t* status;
        
        common::uint64_t* recvret;
        common::uint64_t* recvprod;
        common::uint64_t* sendr;
        
        common::uint64_t port;
        common::uint64_t recvreti, recvprodi, sendri, sendcleani;
        common::uint8_t** sends; // TODO This used to be of type Block, i changed it to uint8_t* so i could compile, but i should figure out what block was.
        int active, duplex;
      };
      
      class broadcom_BCM5751 : public EthernetDriver, public hardware::InterruptHandler {
      private:
        
      public:
        broadcom_BCM5751(sys::hardware::PeripheralComponentDeviceDescriptor* device, hardware::BaseAddressRegister* bar, sys::hardware::InterruptManager* interruptManager);
        ~broadcom_BCM5751();
        
        virtual void activate();
        virtual int reset();
        
        virtual common::uint32_t handleInterrupt(common::uint32_t esp);
        
        virtual void send(common::uint8_t* buffer, int size);
        virtual void receive();
        virtual common::uint64_t getMacAddress();
        
        virtual common::uint32_t getIpAddress();
        virtual void setIpAddress(common::uint32_t ip);
      };
    }
  }
    
#endif
