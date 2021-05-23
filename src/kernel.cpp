#include <common/types.h>
#include <gdt.h>
#include <memorymanagement/memorymanagement.h>
#include <memorymanagement/pagemanagement.h>

#include <hardware/interrupts.h>
#include <hardware/pci.h>

#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/net/ethernet_driver.h>

#include <audio/Audio.h>

#include <filesystem/partition/mbr.h>

#include <net/NetworkManager.h>

#include <multitasking.h>

#include <systemcalls.h>

#include <timer.h>
#include <common/string.h>
#include <cli/cli.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;
using namespace sys::filesystem::partition;
using namespace sys::net;
using namespace sys::audio;

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

uint32_t swapEndian32(uint32_t n) {
  return ((n & 0xFF000000) >> 24)
       | ((n & 0x00FF0000) >> 8)
       | ((n & 0x0000FF00) << 8)
       | ((n & 0x000000FF) << 24);
}

uint16_t SwapEndian16(uint16_t n) {
  return ((n << 8) & 0xFF00)
       | ((n >> 8) & 0xFF);
}

void printf(const char* str) {
  static uint16_t* videoMemory = (uint16_t*)0xB8000;
  static uint8_t x = 0, y = 0;
  
  // First byte in videoMemory is color code, we copy the original color code and add a char.
  for(int i = 0; str[i] != '\0'; i++) {
      switch(str[i]) {
          case '\n':
            x = 0;
            y++;
            break;
          case '\b':
            if(x != 0) {
              x--;
              videoMemory[y*80+x] = (videoMemory[y*80+x] & 0xFF00) | ' ';
            }
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
    char* txt = "00";
    static char* hex = "0123456789ABCDEF";
    
    txt[0] = hex[(num >> 4) & 0xF];
    txt[1] = hex[num & 0xF];
    
    printf(txt);
}

void printHex32(uint32_t num) {
    char* txt = "00000000";
    static char* hex = "0123456789ABCDEF";
    
    txt[0] = hex[(num >> 28) & 0xF];
    txt[1] = hex[(num >> 24) & 0xF];
    txt[2] = hex[(num >> 20) & 0xF];
    txt[3] = hex[(num >> 16) & 0xF];
    txt[4] = hex[(num >> 12) & 0xF];
    txt[5] = hex[(num >> 8) & 0xF];
    txt[6] = hex[(num >> 4) & 0xF];
    txt[7] = hex[num & 0xF];
    
    printf(txt);
}

void printHex64(uint64_t num) {
  printHex32(num >> 32);
  printHex32(num);
}

void printNum(uint32_t num) {
  char chars[33]; // Actually it's less, because we convert to base 10.

  int i = 0;
  do {
    chars[32 - 1 - i] = '0' + (num % 10);
    num /= 10;
    i++;
  } while(i < 32 && num != 0);
  chars[32] = 0;
  printf(&(chars[32 - i]));
}

cli::Cli* myCli;

class PrintKeyboardHandler:public KeyboardEventHandler {
public:
  void onKeyDown(char c) {
    if(myCli != 0) {
      myCli->onKeyDown((uint8_t) c);
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

void sysCall(uint32_t eax, uint32_t ebx) {
  asm("int $0x80" : : "a" (eax), "b" (ebx));
}

void taskA() {
  printf("Checking connection...");
  int attempts = 0;
  bool hasLink = false;
  while(attempts < 15 && !hasLink) {
    hasLink =  NetworkManager::networkManager->hasLink();
    SystemTimer::activeTimer->sleep(1000);
    attempts++;
  }
  if(hasLink) {
    printf(" connected!\n");
  } else {
    printf(" no connection!\n");
  }

  SystemTimer::sleep(1000);

  myCli = new cli::Cli();
  myCli->run();
  delete myCli;
  myCli = 0;
  while(true) {
    asm("hlt");
  }
}

void taskB() {
  while(true) {
    //char* txt = "|";
    //sysCall(0x04, (uint32_t) txt); // 4 is printf.
    asm("hlt");
  }
}

float sqrt(float num) {
  float sqrt = num/2;
  float temp = 0;
  while(sqrt != temp) {
    temp = sqrt;
    sqrt = ( num/temp + temp) / 2;
  }
  return sqrt;
}

extern "C" void kernelMain(void* multiboot_structure, uint32_t magicNumber) {
    for(uint8_t y = 0; y < 25; y++)
        for(uint8_t x = 0; x < 80; x++) 
            printf(" ");
    printf("\n");

    GlobalDescriptorTable gdt;
    
    size_t heap = 10*1024*1024;
    
    uint32_t freeMemory = *(uint32_t*) (((size_t) multiboot_structure)+8);
    freeMemory = freeMemory * 1024 - heap - (10*1024);
    
    MemoryManager memoryManager(heap, freeMemory);
    
    printf("Detected ");
    printNum(freeMemory);
    printf(" bytes of free memory. Heap starts at: 0x");
    printHex32(heap);
    printf("\n");

    new NetworkManager();

    TaskManager taskManager;
    Task* task1 = new Task(&gdt, taskA);
    Task* task2 = new Task(&gdt, taskB);
    
    taskManager.addTask(task1);
    taskManager.addTask(task2);
    
    PageManager pageManager;

    printf("Setting up Drivers\n");
    InterruptManager interrupts(&gdt, &taskManager);
    SystemCallHandler systemCallHandler(&interrupts);

    Audio audioManager;
    
    PartitionManager partitionManager;
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
    
    new SystemTimer();
    
    interrupts.enableInterrupts();
    driverManager.activateAll();
    
    printf("Setting up networking...\n");
    NetworkManager::networkManager->setup();
    
    taskManager.enable();
    while(true);
}
