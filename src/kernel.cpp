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

uint32_t decToInt(char* num) {
  uint32_t ret = 0;
  for(int i = 0; i < 11 & num[i] != 0; i++) {
    ret = ret*10+(num[i]-('0'));
  }
  return ret;
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

class Program {
private:
  TransmissionControlProtocolSocket* mySocket;
  TransmissionControlProtocolProvider* myTcpProvider;
  uint8_t* chars;
  uint32_t current;
  
  uint8_t command;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t x;
  uint8_t y;
public:
  Program(TransmissionControlProtocolProvider* backend) {
    this->myTcpProvider = backend;
    mySocket = 0;
    chars = (uint8_t*) MemoryManager::activeMemoryManager->malloc(1024);
    this->current = 0;
    this->r = 0;
    this->g = 0;
    this->b = 0;
    this->x = 0;
    this->y = 0;
    this->command = 0;
    printf("IP:");
  }
  ~Program() {
    if(mySocket != 0) {
      mySocket->disconnect();
      MemoryManager::activeMemoryManager->free(mySocket);
      mySocket = 0; // Remove pointer.
    }
    MemoryManager::activeMemoryManager->free(chars);
  }
  void loop() {
    char* msg = "Got it";
    uint32_t recvd = 0;
    while(mySocket == 0);
    while(mySocket != 0 && !mySocket->isClosed()) {
      if(mySocket->hasNext() != 0) {
        uint32_t bytesToRead = mySocket->hasNext();
        uint8_t* data = (uint8_t*) MemoryManager::activeMemoryManager->malloc(bytesToRead);
        mySocket->readNext(data, bytesToRead);
        for(int i = 0; i < bytesToRead; i++) {
          if(data[i] == 175 && this->command > 4) {
            this->command = 0;
            continue;
          }
          switch(this->command) {
            default:
              break;
            case 0:
              this->r = data[i];
              break;
            case 1:
              this->g = data[i];
              break;
            case 2:
              this->b = data[i];
              break;
            case 3:
              this->x = data[i];
              break;
            case 4:
              this->y = data[i];
              if(this->x < 320 && this->y < 200) {
                getVGA()->putPixel(this->x, this->y, this->r, this->g, this->b);
                recvd++;
              }
              break;
          }
          this->command = this->command+1;
          if(recvd > 25) {
            recvd = 0;
            mySocket->send((uint8_t*) msg, 6);
          }
        }
        MemoryManager::activeMemoryManager->free(data);
      } else {
        mySocket->sendExpiredPackets();
        asm("hlt"); // Nothing to do.
      }
    }
  }
  void onKeyDown(uint8_t key) {
    if(mySocket == 0) {
      if(key == '\n') {
        chars[current] = 0;
        uint32_t ip = decToInt((char*)chars);
        ip = ((ip & 0xFF000000) >> 24) | ((ip & 0x00FF0000) >> 8) | ((ip & 0x0000FF00) << 8) | ((ip & 0x000000FF) << 24);
        
        printf("I'm at: ");
        printHex32(ipv4->getIpAddress());
        printf("\n");
        
        mySocket = myTcpProvider->connect(ip, 1234);
        printf("Connecting to ");
        printHex32(ip);
        printf(" on port 1234\n");
        current = 0;
        
        enableVGA();
      } else {
        chars[current] = key;
        current++;
        if(current > 10 || key == 'r') {
          current = 0;
          printf("resetting\nIP:");
        }
      }
    } else {
      chars[current] = key;
      current++;
      if(current >= 1024 || key == '\n') {
        mySocket->send(chars, current);
        current = 0;
        printf("SENDING DATA\n");
      }
    }
  }
};

Program* myProgram;

class PrintKeyboardHandler:public KeyboardEventHandler {
public:
  void onKeyDown(char c) {
    char* txt = " ";
    txt[0] = c;
    printf(txt);
    if(myProgram != 0) {
      myProgram->onKeyDown((uint8_t) c);
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

uint64_t getTime() {
  return SystemTimer::getTimeInInterrupts();
}

void taskA() {
  uint64_t lastTime = getTime();
  uint64_t lastSec = 0;
  while(lastSec < 1) { // Let timer run for 10 secondss.
    if(lastTime+18 < getTime()) {
      lastTime += 18;
      lastSec++;
      printf("PIT: ");
      printHex32(lastTime);
      printf("   in seconds: ");
      printHex32(lastSec);
      printf("\n");
    }
  }
  if(tcp != 0) {
    while(ipv4->getIpAddress() == 0) {}
    
    myProgram = new Program(tcp);
    myProgram->loop();
  } else {
    printf("TCP == 0\n");
  }
  while(true) {
    //char* txt = "_";
    //sysCall(0x04, (uint32_t) txt); // 4 is printf.
    //printf("_");
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
    
    printf("Networking...");
    EtherFrameProvider* etherframe = 0;
    if(currentEthernetDriver != 0) {
      etherframe = new EtherFrameProvider(currentEthernetDriver);
      arp = new AddressResolutionProtocol(etherframe);
      ipv4 = new InternetProtocolV4Provider(etherframe, arp); // 0x00FFFFFF = 255.255.255.0 (subnet mask)
      icmp = new InternetControlMessageProtocol(ipv4);
      udp = new UserDatagramProtocolProvider(ipv4);
      tcp = new TransmissionControlProtocolProvider(ipv4);
      dhcp = new DynamicHostConfigurationProtocol(udp, ipv4);
      dhcp->sendDiscover();
    } else {
      if(nicName != 0) {
        printf("NO SOPPURT: ");
        printf(nicName);
      } else {
        printf("NO DRIVER REGISTERED!");
      }
      printf("\n");
    }
    
    printf("Enabling interrupts...\n");
    interrupts.enableInterrupts();
    
    driverManager.activateAll();
    
    taskManager.enable();
    
    while(true);
}
