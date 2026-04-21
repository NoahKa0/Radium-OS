#include <cli/color.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::net;
using namespace sys::drivers;

void printf(const char*);
void printNum(uint32_t);
void enableVGA();
void disableVGA();
VideoGraphicsArray* getVGA();

CmdColor::CmdColor() {
}
CmdColor::~CmdColor() {
}
void setColor(uint16_t color);
void CmdColor::run(common::String** args, common::uint32_t argsLength) {
  if(argsLength < 2) {
    printf("Usage: <colorCodeFG> <colorCodeBG>\n");
    return;
  }

  uint16_t colorCode = (args[0]->parseInt() & 0xF) | ((args[1]->parseInt() & 0xF) << 4);

  setColor(colorCode);
}

void CmdColor::onInput(common::String* input) {

}
