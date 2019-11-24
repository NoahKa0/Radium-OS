#include <drivers/vga.h>

using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

// Constructor
VideoGraphicsArray::VideoGraphicsArray()
: miscPort(0x3c2),
  ctrlIndexPort(0x3d4),
  ctrlDataPort(0x3d5),
  sequencerIndexPort(0x3c4),
  sequencerDataPort(0x3c5),
  graphicsControllerIndexPort(0x3ce),
  graphicsControllerDataPort(0x3cf),
  attributeControllerIndexPort(0x3c0),
  attributeControllerReadPort(0x3c1),
  attributeControllerWritePort(0x3c0),
  attributeControllerResetPort(0x3ca) {}

// Destructor
VideoGraphicsArray::~VideoGraphicsArray() {}

void VideoGraphicsArray::writeRegisters(uint8_t* registers) {
  miscPort.write(*registers);
  *registers++;
  for(uint8_t i = 0; i < 5; i++) {
    sequencerIndexPort.write(i);
    sequencerDataPort.write(*registers);
    *registers++;
  }
  
  ctrlIndexPort.write(0x03);
  ctrlDataPort.write(ctrlDataPort.read() | 0x80);
  ctrlIndexPort.write(0x11);
  ctrlDataPort.write(ctrlDataPort.read() & ~0x80);

  registers[0x03] = registers[0x03] | 0x80;
  registers[0x11] = registers[0x11] & ~0x80;
  
  for(uint8_t i = 0; i < 25; i++) {
    ctrlIndexPort.write(i);
    ctrlDataPort.write(*registers);
    *registers++;
  }
  
  for(uint8_t i = 0; i < 9; i++) {
    graphicsControllerIndexPort.write(i);
    graphicsControllerDataPort.write(*registers);
    *registers++;
  }
  
  for(uint8_t i = 0; i < 21; i++) {
    attributeControllerResetPort.read();
    attributeControllerIndexPort.write(i);
    attributeControllerWritePort.write(*registers);
    *registers++;
  }
  
  attributeControllerResetPort.read();
  attributeControllerIndexPort.write(0x20);
}

bool VideoGraphicsArray::supportsMode(uint32_t width, uint32_t height, uint32_t colorDepth) {
  return (width == 320) && (height == 200) && (colorDepth == 8);
}

bool VideoGraphicsArray::setMode(uint32_t width, uint32_t height, uint32_t colorDepth) {
  if(!supportsMode(width, height, colorDepth)) {
    return false;
  }

  unsigned char g_320x200x256[] =
  {
    /* MISC */
      0x63,
    /* SEQ */
      0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC */
      0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
      0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
      0xFF,
    /* GC */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
      0xFF,
    /* AC */
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x41, 0x00, 0x0F, 0x00, 0x00
  };

  writeRegisters(g_320x200x256);
  return true;
}

void VideoGraphicsArray::putPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
  putPixel(x, y, getColorIndex(r, g, b));
}

uint8_t* VideoGraphicsArray::getFrameBufferSegment() {
  graphicsControllerIndexPort.write(0x06);
  uint8_t segmentNumber = graphicsControllerDataPort.read() & (3<<2);
  switch(segmentNumber)
  {
    default:
    case 0<<2: return (uint8_t*)0x00000;
    case 1<<2: return (uint8_t*)0xA0000;
    case 2<<2: return (uint8_t*)0xB0000;
    case 3<<2: return (uint8_t*)0xB8000;
  }
}

void VideoGraphicsArray::putPixel(uint32_t x, uint32_t y, uint8_t color) {
  uint8_t* pixelAddress = getFrameBufferSegment() + 320*y+x;
  *pixelAddress = color;
}

uint8_t VideoGraphicsArray::getColorIndex(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t rgb = 0;
  uint8_t light = 0;
  if(r > 50) rgb |= 0b100;
  if(g > 50) rgb |= 0b010;
  if(b > 50) rgb |= 0b001;
  
  if(r > 200) rgb |= (0b100 << 3);
  if(g > 200) rgb |= (0b010 << 3);
  if(b > 200) rgb |= (0b001 << 3);
  
  return rgb;
}
