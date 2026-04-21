#ifndef __SYS__CLI__WBMP_H
#define __SYS__CLI__WBMP_H

  #include <cli/cli.h>
  #include <common/types.h>
  #include <common/string.h>
  #include <net/NetworkManager.h>
  #include <net/http.h>
  #include <drivers/vga.h>

  namespace sys {
    namespace cli {
      class CmdWBMP : public Cmd {
      private:
        net::tcp::TransmissionControlProtocolSocket* socket;
        common::uint32_t pressedEnter;

        struct BMPFileHeader {
          common::uint16_t bfType;      // must be 'BM' (0x4D42)
          common::uint32_t bfSize;
          common::uint16_t bfReserved1;
          common::uint16_t bfReserved2;
          common::uint32_t bfOffBits;   // offset to pixel data
        }  __attribute__((packed));

        struct DIBHeader {
          common::uint32_t biSize;       // should be 40
          common::int32_t  width;
          common::int32_t  height;
          common::uint16_t planes;
          common::uint16_t bitCount;     // 24 or 32 most common
          common::uint32_t compression;  // should be 0 (BI_RGB = no compression)
        }  __attribute__((packed));

      public:
        CmdWBMP();
        ~CmdWBMP();

        void showBmp(common::uint8_t* data, common::uint32_t length);

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onInput(common::String* input);
      };
    }
  }

#endif