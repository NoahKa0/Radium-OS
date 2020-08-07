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
          common::uint32_t ampGainSteps;
        };
        struct HDA_BDLE {
          common::uint64_t address;
          common::uint32_t length;
          common::uint32_t flags;
        } __attribute__((packed));

        class HDA : public Driver, public hardware::InterruptHandler {
        private:
          enum {
            Gctl        = 8,
            Rst         = 1,
            WakeEn      = 12,
            Statests    = 14,
            SSYNC       = 56,
            Corblbase   = 64,
            Corbubase   = 68,
            Corbwp      = 72,
            Corbrp      = 74,
            Corbdma     = 2,
            Corbptrrst  = 32768,
            Corbctl     = 76,
            Rirbctl     = 92,
            Rirbsts     = 93,
            Rirblbase   = 80,
            Rirbubase   = 84,
            Rirbwp      = 88,
            Rirbdma     = 2,
            Rirbptrrst  = 32768,
            Corbsz      = 78,
            Rirbsz      = 94,
            Dplbase     = 112,
            Dpubase     = 116,
            Intctl      = 32,
            SDnCTL      = 0x100,
            SDnCBL      = 8,
            SDnLVI      = 12,
            SDnFMT      = 18,
            SDnBDPL     = 24,
            SDnBDPU     = 28
          };

          enum widget {
            WIDGET_OUTPUT           = 0x0,
            WIDGET_INPUT            = 0x1,
            WIDGET_MIXER            = 0x2,
            WIDGET_SELECTOR         = 0x3,
            WIDGET_PIN              = 0x4,
            WIDGET_POWER            = 0x5,
            WIDGET_VOLUME_KNOB      = 0x6,
            WIDGET_BEEP_GEN         = 0x7,
            WIDGET_VENDOR_DEFINED   = 0xf,
          };

          enum verb {
            VERB_GET  = 0xF00,
            VERB_SET_POWER_STATE = 0x705,
            VERB_SET_STREAM_CHANNEL = 0x706,
            VERB_GET_PIN_CTL = 0xF07,
            VERB_SET_PIN_CTL = 0x707,
            VERB_GET_EAPD_BTL = 0xF0C,
            VERB_SET_EAPD_BTL = 0x70C,
            VERB_SET_FORMAT = 0x200,
            VERB_SET_AMP_GAIN_MUTE = 0x300
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

          void setNode(int node);

          common::uint64_t corb_write_cmd(common::uint32_t cad, common::uint32_t nid, common::uint32_t verb, common::uint32_t cmd);

          bool storeNode(common::uint32_t cad, common::uint32_t nid, common::uint32_t ampGainSteps);
          void collectAFGNodes();
          void initWidget(common::uint32_t cad, common::uint32_t nid);
        public:
          HDA(sys::hardware::PeripheralComponentDeviceDescriptor* device, sys::hardware::InterruptManager* interruptManager);
          ~HDA();

          void setVolume(common::uint8_t volume);

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