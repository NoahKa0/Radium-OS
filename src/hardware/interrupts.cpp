#include <hardware/interrupts.h>

using namespace sys::hardware;
using namespace sys::common;
using namespace sys;

void printf(char* str);
void printHex8(uint8_t num);


InterruptHandler::InterruptHandler(uint8_t interruptNumber, InterruptManager* interruptManager) {
    this->interruptManager = interruptManager;
    this->interruptNumber = interruptNumber;
    interruptManager->handlers[interruptNumber] = this;
}

InterruptHandler::~InterruptHandler() {
    if(interruptManager->handlers[interruptNumber] == this) {
        interruptManager->handlers[interruptNumber] = 0;
    }
}

uint32_t InterruptHandler::handleInterrupt(uint32_t esp) {
    return esp;
}


InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];

InterruptManager* InterruptManager::activeInterruptManager = 0;

void InterruptManager::setInterruptDescriptorTableEntry(
    uint8_t interruptNumber,
    uint16_t codeSegmentSelectorOffset,
    void (*handler)(),
    uint8_t descriptorPrivilegeLevel,
    uint8_t descriptorType)
{
    interruptDescriptorTable[interruptNumber].handlerAdressLowBits = ((uint32_t) handler) & 0xFFFF;
    
    interruptDescriptorTable[interruptNumber].handlerAdressHighBits = (((uint32_t) handler) >> 16) & 0xFFFF;
    
    interruptDescriptorTable[interruptNumber].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
    
    const uint8_t IDT_DESC_PRESENT = 0x80;
    interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | ((descriptorPrivilegeLevel & 3) << 5) | descriptorType;
    
    interruptDescriptorTable[interruptNumber].reserved = 0;
}

InterruptManager::InterruptManager(GlobalDescriptorTable* gdt)
: picMasterCommand(0x20),
picMasterData(0x21),
picSlaveCommand(0xA0),
picSlaveData(0xA1)
{
    uint16_t codeSegment = gdt->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;
    
    for(uint16_t i = 0; i < 256; i++) {
        handlers[i] = 0;
        
        setInterruptDescriptorTableEntry(i, codeSegment, &ignoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }
    
    setInterruptDescriptorTableEntry(0x00, codeSegment, &handleException0x00, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x01, codeSegment, &handleException0x01, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x02, codeSegment, &handleException0x02, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x03, codeSegment, &handleException0x03, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x04, codeSegment, &handleException0x04, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x05, codeSegment, &handleException0x05, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x06, codeSegment, &handleException0x06, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x07, codeSegment, &handleException0x07, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x08, codeSegment, &handleException0x08, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x09, codeSegment, &handleException0x09, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x0A, codeSegment, &handleException0x0A, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x0B, codeSegment, &handleException0x0B, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x0C, codeSegment, &handleException0x0C, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x0D, codeSegment, &handleException0x0D, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x0E, codeSegment, &handleException0x0E, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x0F, codeSegment, &handleException0x0F, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x10, codeSegment, &handleException0x10, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x11, codeSegment, &handleException0x11, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x12, codeSegment, &handleException0x12, 0, IDT_INTERRUPT_GATE);
    
    setInterruptDescriptorTableEntry(0x20, codeSegment, &handleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x21, codeSegment, &handleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x22, codeSegment, &handleInterruptRequest0x02, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x23, codeSegment, &handleInterruptRequest0x03, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x24, codeSegment, &handleInterruptRequest0x04, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x25, codeSegment, &handleInterruptRequest0x05, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x26, codeSegment, &handleInterruptRequest0x06, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x27, codeSegment, &handleInterruptRequest0x07, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x28, codeSegment, &handleInterruptRequest0x08, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x29, codeSegment, &handleInterruptRequest0x09, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x2A, codeSegment, &handleInterruptRequest0x0A, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x2B, codeSegment, &handleInterruptRequest0x0B, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x2C, codeSegment, &handleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x2D, codeSegment, &handleInterruptRequest0x0D, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x2E, codeSegment, &handleInterruptRequest0x0E, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x2F, codeSegment, &handleInterruptRequest0x0F, 0, IDT_INTERRUPT_GATE);
    setInterruptDescriptorTableEntry(0x51, codeSegment, &handleInterruptRequest0x31, 0, IDT_INTERRUPT_GATE);
    
    
    picMasterCommand.Write(0x11);
    picSlaveCommand.Write(0x11);
    
    // One pic can have 8 interrupts
    picMasterData.Write(0x20); // Master pic gets interrupts 20-27
    picSlaveData.Write(0x28); // Slave pic gets interrupts 28-35
    
    // Not a clue what this does the tutorial says it tells the master and slave pic who is who.
    picMasterData.Write(0x04); // Tells the master pic that is the master (i think)
    picSlaveData.Write(0x02); // Tells the slave pic that is the slave (i think)
    
    picMasterData.Write(0x01);
    picSlaveData.Write(0x01);
    
    picMasterData.Write(0x00);
    picSlaveData.Write(0x00);
    
    interruptDescriptorTablePointer idt_pointer;
    idt_pointer.size = 256 * sizeof(GateDescriptor) - 1;
    idt_pointer.base = (uint32_t) interruptDescriptorTable;
    
    asm volatile("lidt %0" : : "m" (idt_pointer));
}

InterruptManager::~InterruptManager() {}

void InterruptManager::enableInterrupts() {
    if(activeInterruptManager != 0) {
        activeInterruptManager->disableInterrupts();
    }
    activeInterruptManager = this;
    asm("sti");
}

void InterruptManager::disableInterrupts() {
    if(activeInterruptManager == this) {
        activeInterruptManager = 0;
        asm("cli");
    }
}

// esp is the current stack pointer.
uint32_t InterruptManager::onInterrupt(uint8_t interruptNumber, uint32_t esp) {
    if(activeInterruptManager != 0) {
        return activeInterruptManager->handleInterrupt(interruptNumber, esp);
    }
    return esp;
}

// esp is the current stack pointer.
uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    if(handlers[interruptNumber] != 0) {
        esp = handlers[interruptNumber]->handleInterrupt(esp);
    } else if(interruptNumber != 0x20) {
        char* txt = "Unhandled interrupt: ";
        printHex8(interruptNumber);
        printf("\n");
    }
    
    // Interrupts 0x20-0x30 are generated by the pic.
    if(interruptNumber >= 0x20 && interruptNumber <= 0x30) {
        picMasterCommand.Write(0x20);
        
        // All interrupts above 0x28 are from the slave pic.
        // I don't know if i can ignore the master pic but from what i could find everyone answered the master pic aswell.
        if(interruptNumber >= 0x28) {
            picSlaveCommand.Write(0x20);
        }
    }
    return esp;
}
