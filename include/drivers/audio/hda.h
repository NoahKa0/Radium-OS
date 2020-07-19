#ifndef __MYOS__DRIVERS__AUDIO__HDA_H
#define __MYOS__DRIVERS__AUDIO__HDA_H

  #include <common/types.h>
  #include <drivers/driver.h>
  #include <hardware/pci.h>
  #include <hardware/interrupts.h>
  #include <memorymanagement/memorymanagement.h>

  namespace sys {
    namespace drivers {
      namespace audio {
        struct HDA_node {
          common::uint32_t cad;
          common::uint32_t nid;
        };
        struct HDA_BDLE {
          common::uint64_t address;
          common::uint32_t length;
          common::uint8_t IOC;
          common::uint8_t reserved1;
          common::uint16_t reserved2;
        } __attribute__((packed));
        class HDA : public Driver, public hardware::InterruptHandler {
        private:
          enum {
            Gctl = 8,
            Rst = 1,
            Statests = 14,
            Corblbase = 64,
            Corbubase = 68,
            Corbwp = 72,
            Corbrp = 74,
            Corbdma = 2,
            Corbptrrst = 32768,
            Corbctl = 76,
            Rirbctl = 92,
            Rirbsts = 93,
            Rirblbase = 80,
            Rirbubase = 84,
            Rirbwp = 88,
            Rirbdma = 2,
            Rirbptrrst = 32768,
            Corbsz = 78,
            Rirbsz = 94,
            Dplbase = 112,
            Dpubase = 116,
            Intctl = 32,
            SDnCTL = 128,
            SDnCBL = 136,
            SDnLVI = 140,
            SDnFMT = 146,
            SDnBDPL = 152,
            SDnBDPU = 156
          };
          common::uint8_t* baseAddr;
          common::uint32_t basePort;
          sys::hardware::PeripheralComponentDeviceDescriptor* device;

          common::uint32_t corbSize;
          common::uint32_t* corb;

          common::uint32_t rirbSize;
          common::uint64_t* rirb;
          common::uint32_t rirbRp;

          common::uint32_t maxNodes = 16;
          HDA_node nodes[16];

          common::uint32_t bdl_length;
          HDA_BDLE* bdl_start;

          void setupBDL();

          void corb_write_cmd(common::uint32_t cad, common::uint32_t nid, common::uint32_t verb, common::uint32_t cmd);

          bool storeNode(common::uint32_t cad, common::uint32_t nid);
          void collectAFGNodes();
          common::uint64_t getNextSolicitedResponse();
          void cmd_reset();
        public:
          HDA(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
          ~HDA();

          virtual void activate();
          virtual int reset();
          virtual void deactivate();
          
          virtual common::uint32_t handleInterrupt(common::uint32_t esp);

          static Driver* getDriver(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
        };
      }
    }
  }

#endif