#ifndef __SYS__CLI__SNAKE_H
#define __SYS__CLI__SNAKE_H

  #include <cli/cli.h>
  #include <common/types.h>

  namespace sys {
    namespace cli {
      class CmdSnake : public Cmd {
      private:
        common::uint32_t* snake;
        common::uint32_t length;
        common::uint8_t foodX;
        common::uint8_t foodY;

        common::uint8_t direction;
        enum {
          UP = 0,
          DOWN = 1,
          RIGHT = 2,
          LEFT = 3
        };
        enum {
          WIDTH = 80,
          HEIGHT = 25
        };
      public:
        CmdSnake();
        ~CmdSnake();

        virtual void run(common::String** args, common::uint32_t argsLength);
        virtual void onKeyDown(char c);

        common::uint8_t toX(common::uint32_t c);
        common::uint8_t toY(common::uint32_t c);
        common::uint32_t toCoord(common::uint8_t x, common::uint8_t y);

        common::uint32_t lcg(common::uint32_t n);
      };
    }
  }

#endif