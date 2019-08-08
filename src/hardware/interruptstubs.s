.section .text

.extern _ZN3sys8hardware16InterruptManager11onInterruptEhj

.global _ZN3sys8hardware16InterruptManager22ignoreInterruptRequestEv

.macro handleException num
.global _ZN3sys8hardware16InterruptManager19handleException\num\()Ev
_ZN3sys8hardware16InterruptManager19handleException\num\()Ev:
    movb $\num, (interruptnumber)
    jmp int_handler
.endm

handleException 0x00
handleException 0x01
handleException 0x02
handleException 0x03
handleException 0x04
handleException 0x05
handleException 0x06
handleException 0x07
handleException 0x08
handleException 0x09
handleException 0x0A
handleException 0x0B
handleException 0x0C
handleException 0x0D
handleException 0x0E
handleException 0x0F
handleException 0x10
handleException 0x11
handleException 0x12

.macro handleInterruptRequest num
.global _ZN3sys8hardware16InterruptManager26handleInterruptRequest\num\()Ev
_ZN3sys8hardware16InterruptManager26handleInterruptRequest\num\()Ev:
    movb $\num, (interruptnumber)
    pushl $0
    jmp int_handler
.endm

handleInterruptRequest 0x20
handleInterruptRequest 0x21
handleInterruptRequest 0x22
handleInterruptRequest 0x23
handleInterruptRequest 0x24
handleInterruptRequest 0x25
handleInterruptRequest 0x26
handleInterruptRequest 0x27
handleInterruptRequest 0x28
handleInterruptRequest 0x29
handleInterruptRequest 0x2A
handleInterruptRequest 0x2B
handleInterruptRequest 0x2C
handleInterruptRequest 0x2D
handleInterruptRequest 0x2E
handleInterruptRequest 0x2F
handleInterruptRequest 0x51

int_handler:
    # add all registers to the stack so they can be restored later.
    pushl %ebp;
    pushl %edi;
    pushl %esi;
    
    pushl %edx;
    pushl %ecx;
    pushl %ebx;
    pushl %eax;
    
    # Call onInterrupt in C++;
    pushl %esp
    push (interruptnumber)
    call _ZN3sys8hardware16InterruptManager11onInterruptEhj
    movl %eax, %esp
    
    # restore all registers so we can continue doing what we were doing before the interrupt came to bother us.
    popl %eax;
    popl %ebx;
    popl %ecx;
    popl %edx;

    popl %esi;
    popl %edi;
    popl %ebp;
    
    add $4, %esp;
    
_ZN3sys8hardware16InterruptManager22ignoreInterruptRequestEv:
    
    iret
    
    
.data
    interruptnumber: .byte 0
