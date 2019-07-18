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
    commandPort.Write(0xA8);
    
    commandPort.Write(0x20); // Get current state
    uint8_t status = dataPort.Read() | 2;
    
    commandPort.Write(0x60); // Set state
    
    dataPort.Write(status);
    
    commandPort.Write(0xD4);
    dataPort.Write(0xF4);
    
    dataPort.Read();
}

uint32_t MouseDriver::handleInterrupt(uint32_t esp) {
    uint8_t status = commandPort.Read();
    
    if(!(status & 0x20) || handler == 0) {
        return esp;
    }
    
    buffer[offset] = dataPort.Read();
    
    offset = (offset + 1) % 3;
    
    if(offset == 0) {
        int x = buffer[1];
        int y = -buffer[2];
        handler->onMove(x, y);
    }
    
    return esp;
}
