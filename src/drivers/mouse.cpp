#include <drivers/mouse.h>

using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

void printf(char* str);

MouseEventHandler::MouseEventHandler() {}
MouseEventHandler::~MouseEventHandler() {}

void MouseEventHandler::onMouseDown() {}
void MouseEventHandler::onMouseUp() {}

void MouseEventHandler::onMove(int x, int y) {}


MouseDriver::MouseDriver(InterruptManager* interruptManager, MouseEventHandler* handler)
:InterruptHandler(0x2C, interruptManager),
dataPort(0x60),
commandPort(0x64)
{
    offset = 0;
    buttons = 0;
    
    this->handler = handler;
}

MouseDriver::~MouseDriver() {}

void MouseDriver::activate() {
    commandPort.write(0xA8);
    
    commandPort.write(0x20); // Get current state
    uint8_t status = dataPort.read() | 2;
    
    commandPort.write(0x60); // Set state
    
    dataPort.write(status);
    
    commandPort.write(0xD4);
    dataPort.write(0xF4);
    
    dataPort.read();
}

uint32_t MouseDriver::handleInterrupt(uint32_t esp) {
    uint8_t status = commandPort.read();
    
    if(!(status & 0x20) || handler == 0) {
        return esp;
    }
    
    buffer[offset] = dataPort.read();
    
    offset = (offset + 1) % 3;
    
    if(offset == 0) {
        int x = buffer[1];
        int y = -buffer[2];
        handler->onMove(x, y);
    }
    
    return esp;
}
