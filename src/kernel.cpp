#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>

#include <hardware/interrupts.h>
#include <hardware/pci.h>

#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>

#include <multitasking.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;

static VideoGraphicsArray* videoGraphicsArray = 0;
static bool videoEnabled = false;

VideoGraphicsArray* getVGA() {
  if(!videoEnabled) return 0;
  return videoGraphicsArray;
}

void enableVGA() {
  if(!videoEnabled && videoGraphicsArray != 0) {
    videoEnabled = true;
    videoGraphicsArray->setMode(320, 200, 8);
    
    for(int x = 0; x < 320; x++) {
      for(int y = 0; y < 200; y++) {
        videoGraphicsArray->putPixel(x, y, 0, 0, 42);
      }
    }
  }
}

void printf(char* str) {
  static uint16_t* videoMemory = (uint16_t*)0xB8000;
  static uint8_t x = 0, y = 0;
  
  // First byte in videoMemory is color code, we copy the original color code and add a char.
  for(int i = 0; str[i] != '\0'; i++) {
      switch(str[i]) {
          case '\n':
              x = 0;
              y++;
              break;
          default:
            videoMemory[y*80+x] = (videoMemory[y*80+x] & 0xFF00) | str[i];
            x++;
            break;
      }
    
    if(x >= 80) { // If row is full do a line-break.
        x = 0;
        y++;
    }
    
    while(y >= 24) {
        for(uint8_t ix = 0; ix < 80; ix++) {
            for(uint8_t iy = 0; iy < 25; iy++) {
                if(iy >= 25) {
                    videoMemory[iy*80+ix] = (videoMemory[iy*80+ix] & 0xFF);
                } else {
                    videoMemory[iy*80+ix] = videoMemory[(iy+1)*80+ix];
                }
            }
        }
        y--;
    }
  }
}

void printHex8(uint8_t num) {
    char* txt = "0x00";
    static char* hex = "0123456789ABCDEF";
    
    txt[2] = hex[(num >> 4) & 0xF];
    txt[3] = hex[num & 0xF];
    
    printf(txt);
}

void printHex32(uint32_t num) {
    char* txt = "0x00000000";
    static char* hex = "0123456789ABCDEF";
    
    txt[2] = hex[(num >> 28) & 0xF];
    txt[3] = hex[(num >> 24) & 0xF];
    txt[4] = hex[(num >> 20) & 0xF];
    txt[5] = hex[(num >> 16) & 0xF];
    txt[6] = hex[(num >> 12) & 0xF];
    txt[7] = hex[(num >> 8) & 0xF];
    txt[8] = hex[(num >> 4) & 0xF];
    txt[9] = hex[num & 0xF];
    
    printf(txt);
}

class PrintKeyboardHandler:public KeyboardEventHandler {
public:
  void onKeyDown(char c) {
    char* txt = " ";
    txt[0] = c;
    printf(txt);
    
    if(c == '-') {
      if(!videoEnabled) enableVGA();
      VideoGraphicsArray* vga = getVGA();
      for(int x = 0; x < 320; x++) {
        for(int y = 0; y < 200; y++) {
          if(x > 80 && x < 240 && y > 50 && y < 150) {
            vga->putPixel(x, y, 42, 0, 42);
          } else {
            vga->putPixel(x, y, 0, 0, 42);
          }
        }
      }
    }
  }
};

class MyMouseHandler:public MouseEventHandler {
public:
    void onMove(int x, int y) {
        static uint16_t* videoMemory = (uint16_t*)0xB8000;
        static int8_t mx=40, my=12;
        
        videoMemory[my*80+mx] = ((videoMemory[my*80+mx] << 4) & 0xF000) |
                                ((videoMemory[my*80+mx] >> 4) & 0x0F00) |
                                (videoMemory[my*80+mx] & 0x00FF);
        
        mx += x;
        if(mx < 0) mx = 0;
        if(mx > 80) mx = 80;
        my += y;
        if(my < 0) my = 0;
        if(my > 24) my = 24;
        
        videoMemory[my*80+mx] = ((videoMemory[my*80+mx] << 4) & 0xF000) |
                                ((videoMemory[my*80+mx] >> 4) & 0x0F00) |
                                (videoMemory[my*80+mx] & 0x00FF);
    }
};

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for(constructor* i = &start_ctors; i != &end_ctors; i++) {
        (*i)();
    }
}

void taskA() {
  while(true) {
    //printf("_");
  }
}

void taskB() {
  while(true) {
    //printf("|");
  }
}

extern "C" void kernelMain(void* multiboot_structure, uint32_t magicNumber) {
    for(uint8_t y = 0; y < 25; y++)
        for(uint8_t x = 0; x < 80; x++) 
            printf(" ");
    printf("\n");
    
    printf("Setting up GlobalDescriptorTable\n");
    GlobalDescriptorTable gdt;
    
    size_t heap = 10*1024*1024;
    
    uint32_t freeMemory = *(uint32_t*) (((size_t) multiboot_structure)+8);
    freeMemory = freeMemory * 1024 - heap - (10*1024);
    
    MemoryManager memoryManager(heap, freeMemory);
    
    printf("Detected ");
    printHex32(freeMemory);
    printf(" bytes of free memory. Heap starts at: ");
    printHex32(heap);
    printf("\n");
    
    TaskManager taskManager;
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    
    taskManager.addTask(&task1);
    taskManager.addTask(&task2);
    
    printf("Setting up Drivers\n");
    InterruptManager interrupts(&gdt, &taskManager);
    DriverManager driverManager;
    
    PrintKeyboardHandler keyboardHandler;
    KeyboardDriver keyboard(&interrupts, &keyboardHandler);
    driverManager.addDriver(&keyboard);
    
    MyMouseHandler mouseHandler;
    MouseDriver mouse(&interrupts, &mouseHandler);
    driverManager.addDriver(&mouse);
    
    PeripheralComponentInterconnect PCIController;
    PCIController.selectDrivers(&driverManager, &interrupts);
    
    VideoGraphicsArray vga;
    videoGraphicsArray = &vga;
    
    printf("ATA Primary: master: ");
    AdvancedTechnologyAttachment ataPM(0x1F0, true);
    ataPM.identify();
    printf(" slave: ");
    AdvancedTechnologyAttachment ataPS(0x1F0, false);
    ataPS.identify();
    printf("\n");
    
    printf("ATA Secondary: master: ");
    AdvancedTechnologyAttachment ataSM(0x170, true);
    ataSM.identify();
    printf(" slave: ");
    AdvancedTechnologyAttachment ataSS(0x170, false);
    ataSS.identify();
    printf("\n");
    
    printf("Enabling interrupts...\n");
    
    driverManager.activateAll();
    interrupts.enableInterrupts();
    
    while(true);
}
