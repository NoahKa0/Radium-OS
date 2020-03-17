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
#include <drivers/ata.h>
#include <drivers/net/ethernet_driver.h>

#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/dhcp.h>

#include <multitasking.h>

#include <systemcalls.h>

#include <timer.h>
#include <common/string.h>
#include <test/cli.h>

using namespace sys;
using namespace sys::common;
using namespace sys::drivers;
using namespace sys::hardware;
using namespace sys::net;

static VideoGraphicsArray* videoGraphicsArray = 0;
static bool videoEnabled = false;

// Network
static EthernetDriver* currentEthernetDriver = 0;
static AddressResolutionProtocol* arp = 0;
static InternetProtocolV4Provider* ipv4 = 0;
static InternetControlMessageProtocol* icmp = 0;
static UserDatagramProtocolProvider* udp = 0;
static DynamicHostConfigurationProtocol* dhcp = 0;
static TransmissionControlProtocolProvider* tcp = 0;
static char* nicName = 0; // This is almost wordplay XD (nic = network interface card).

void setNicName(char* name) {
  nicName = name;
}

void setSelectedEthernetDriver(EthernetDriver* drv) {
  currentEthernetDriver = drv;
}

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

InternetControlMessageProtocol* getICMP() {
  return icmp;
}

TransmissionControlProtocolProvider* getTCP() {
  return tcp;
}

test::Cli* myCli;

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
  if(currentEthernetDriver != 0) {
    printf("Waiting for connection...");
    for(int i = 0; i < 15 && !currentEthernetDriver->hasLink(); i++) {
      SystemTimer::sleep(1000);
    }
    if(currentEthernetDriver->hasLink()) {
      printf(" connected!\n");
    } else {
      printf(" no connection!\n");
    }
  }
          
  if(dhcp != 0) {
    dhcp->sendDiscover();
  }
  SystemTimer::sleep(1000);

  myCli = new test::Cli();
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
    Task* task1 = new Task(&gdt, taskA);
    Task* task2 = new Task(&gdt, taskB);
    
    taskManager.addTask(task1);
    taskManager.addTask(task2);
    
    printf("Setting up paging\n");
    PageManager pageManager;

    printf("Setting up Drivers\n");
    InterruptManager interrupts(&gdt, &taskManager);
    SystemCallHandler systemCallHandler(&interrupts);
    
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
    
    printf("Initialising SystemTimer\n");
    new SystemTimer();
    
    printf("Enabling interrupts and activating drivers...\n");
    interrupts.enableInterrupts();
    driverManager.activateAll();
    
    printf("Setting up networking...\n");
    EtherFrameProvider* etherframe = 0;
    if(currentEthernetDriver != 0) {
      etherframe = new EtherFrameProvider(currentEthernetDriver);
      arp = new AddressResolutionProtocol(etherframe);
      ipv4 = new InternetProtocolV4Provider(etherframe, arp); // 0x00FFFFFF = 255.255.255.0 (subnet mask)
      icmp = new InternetControlMessageProtocol(ipv4);
      udp = new UserDatagramProtocolProvider(ipv4);
      tcp = new TransmissionControlProtocolProvider(ipv4);
      dhcp = new DynamicHostConfigurationProtocol(udp, ipv4);
    } else {
      if(nicName != 0) {
        printf("NO SOPPURT: ");
        printf(nicName);
      } else {
        printf("NO DRIVER REGISTERED!");
      }
      printf("\n");
    }
    
    taskManager.enable();
    while(true);
}
