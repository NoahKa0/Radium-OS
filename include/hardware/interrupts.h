#ifndef __SYS__HARDWARE__INTERRUPTS_H
#define __SYS__HARDWARE__INTERRUPTS_H

    #include <common/types.h>
    #include <hardware/port.h>
    #include <gdt.h>
    
    namespace sys {
        namespace hardware {
            
            class InterruptManager;
            
            class InterruptHandler {
            protected:
                sys::common::uint8_t interruptNumber;
                InterruptManager* interruptManager;
                
                InterruptHandler(sys::common::uint8_t interruptNumber, InterruptManager* interruptManager);
                ~InterruptHandler();
                
            public:
                virtual sys::common::uint32_t handleInterrupt(sys::common::uint32_t esp);
            };
            
            class InterruptManager {
            friend class InterruptHandler;
            protected:
                static InterruptManager* activeInterruptManager;
                
                InterruptHandler* handlers[256];
                
                struct GateDescriptor {
                    sys::common::uint16_t handlerAdressLowBits;
                    sys::common::uint16_t gdt_codeSegmentSelector;
                    sys::common::uint8_t reserved;
                    sys::common::uint8_t access;
                    sys::common::uint16_t handlerAdressHighBits;
                    
                } __attribute__((packed));
                
                static GateDescriptor interruptDescriptorTable[256];
                
                struct interruptDescriptorTablePointer {
                    sys::common::uint16_t size;
                    sys::common::uint32_t base;
                } __attribute__((packed));
                
                static void setInterruptDescriptorTableEntry(
                    sys::common::uint8_t interruptNumber,
                    sys::common::uint16_t codeSegmentSelectorOffset,
                    void (*handler)(),
                    sys::common::uint8_t descriptorPrivilegeLevel,
                    sys::common::uint8_t descriptorType
                );
                
                sys::hardware::Port8BitSlow picMasterCommand;
                sys::hardware::Port8BitSlow picMasterData;
                
                sys::hardware::Port8BitSlow picSlaveCommand;
                sys::hardware::Port8BitSlow picSlaveData;
                
            public:
                InterruptManager(sys::GlobalDescriptorTable* gdt);
                ~InterruptManager();
                
                void enableInterrupts();
                void disableInterrupts();
                
                static sys::common::uint32_t onInterrupt(sys::common::uint8_t interruptNumber, sys::common::uint32_t esp);
                sys::common::uint32_t handleInterrupt(sys::common::uint8_t interruptNumber, sys::common::uint32_t esp);
                
                static void ignoreInterruptRequest();
                
                static void handleInterruptRequest0x00();
                static void handleInterruptRequest0x01();
                static void handleInterruptRequest0x02();
                static void handleInterruptRequest0x03();
                static void handleInterruptRequest0x04();
                static void handleInterruptRequest0x05();
                static void handleInterruptRequest0x06();
                static void handleInterruptRequest0x07();
                static void handleInterruptRequest0x08();
                static void handleInterruptRequest0x09();
                static void handleInterruptRequest0x0A();
                static void handleInterruptRequest0x0B();
                static void handleInterruptRequest0x0C();
                static void handleInterruptRequest0x0D();
                static void handleInterruptRequest0x0E();
                static void handleInterruptRequest0x0F();
                static void handleInterruptRequest0x31();
                
                static void handleException0x00();
                static void handleException0x01();
                static void handleException0x02();
                static void handleException0x03();
                static void handleException0x04();
                static void handleException0x05();
                static void handleException0x06();
                static void handleException0x07();
                static void handleException0x08();
                static void handleException0x09();
                static void handleException0x0A();
                static void handleException0x0B();
                static void handleException0x0C();
                static void handleException0x0D();
                static void handleException0x0E();
                static void handleException0x0F();
                static void handleException0x10();
                static void handleException0x11();
                static void handleException0x12();
            };
        }
    }
    
#endif
