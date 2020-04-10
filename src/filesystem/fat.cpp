#include <filesystem/fat.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::filesystem::partition;

void printf(char*);
void printHex8(uint8_t);
void printHex32(uint32_t);

FatFile::FatFile(Fat* fat, bool isFolder, uint32_t firstFileCluster, uint32_t parentCluster, uint32_t size)
:File(fat)
{
  this->fat = fat;
  this->directory = isFolder;
  this->firstFileCluster = firstFileCluster;
  this->nextFileCluster = firstFileCluster;
  this->parentCluster = parentCluster;
  this->lastCluster = 0;
  this->size = size;
  this->readPosition = 0;
  this->sectorOffset = 0;

  uint32_t fileSector = fat->dataStart + fat->sectorsPerCluster * (nextFileCluster-2);
  this->fat->partition->read(fileSector + sectorOffset, this->buffer, 512);
  this->sectorOffset++;
}
FatFile::~FatFile() {

}

bool FatFile::isFolder() {
  return this->directory;
}
File* FatFile::getParent() {
  return 0;
}
File* FatFile::getChild(uint32_t file) {
  if(!this->directory) {
    return 0;
  }
}
File* FatFile::getChildByName(common::String* file) {
  if(!this->directory) {
    return 0;
  }
}
uint32_t FatFile::numChildren() {
  if(!this->directory) {
    return 0;
  }
}
bool FatFile::mkFile(String* filename, bool folder) {
  if(!this->directory) {
    return 0;
  }
}

bool FatFile::append(uint8_t* data, uint32_t length) {
  return false;
}

int FatFile::hasNext() {
  return this->size - this->readPosition;
}

void FatFile::read(uint8_t* buffer, uint32_t length) {

}


Fat::Fat(Partition* partition) {
  this->bpb = (fatBPB*) MemoryManager::activeMemoryManager->malloc(sizeof(fatBPB));
  partition->read(0, (uint8_t*) this->bpb, sizeof(fatBPB));
  this->partition = partition;

  this->fatStart = bpb->reservedSectors;
  this->dataStart = this->fatStart + (bpb->tableSize * bpb->fatCopies);
  this->rootStart = this->dataStart + bpb->sectorsPerCluster*bpb->rootCluster - 2;
  this->sectorsPerCluster = bpb->sectorsPerCluster;

  this->fat = (uint32_t*) MemoryManager::activeMemoryManager->malloc(this->bpb->tableSize * 512);
  uint8_t* fatData = (uint8_t*) this->fat;
  for(int s = 0; s < this->bpb->tableSize; s++) {
    this->partition->read(this->fatStart + s, fatData + (s * 512), 512);
  }
}
Fat::~Fat() {
  this->flushFat();
  this->partition = 0;
  if(this->fat != 0) {
    delete this->fat;
  }
  if(this->bpb != 0) {
    delete this->bpb;
  }
  this->fat = 0;
  this->bpb = 0;
}

File* Fat::getRoot() {
  return 0;
}
String* Fat::getName() {
  return 0;
}
String* Fat::getType() {
  return new String("FAT32");
}

void Fat::flushFat() {
  uint32_t fatSize = this->bpb->tableSize;
  uint64_t sectorOffset = 0;
  uint8_t* fatData = (uint8_t*) this->fat;
  for(int f = 0; f < this->bpb->fatCopies; f++) {
    printf("Fatflush ");
    for(int s = 0; s < fatSize; s++) {
      this->partition->write(this->fatStart + sectorOffset, fatData + (s * 512), 512);
      sectorOffset++;
      printf(".");
    }
    printf(" done!\n");
  }
  printHex32(fatSize);
}

void tmpReadDir(partition::Partition* ata, fatBPB bpb, uint32_t fatStart, uint32_t dataStart, uint32_t dirSector, bool isRoot = true, uint8_t dirDepth = 0) {
  fatDirectoryEntry dirEntry[16];
  ata->read(dirSector, (uint8_t*) &dirEntry[0], 16 * sizeof(fatDirectoryEntry));

  uint8_t buf[513];
  uint8_t* buffer = (uint8_t*) &buf;
  uint8_t fatBuf[513];
  uint8_t* fatBuffer = (uint8_t*) &fatBuf;
  for(int i = 0; i < 16; i++) {
    if(dirEntry[i].name[0] == 0) {
      break;
    }
    if(dirEntry[i].name[0] == 0xE5 || (dirEntry[i].attributes & 0x0F) == 0x0F) {
      continue;
    }
    String* fname = new String((char*) &dirEntry[i].name, 11);
    for(int i = 0; i < dirDepth; i++) printf("  ");
    printf("File: ");
    printf(fname->getCharPtr());
    printf(" attr: ");
    printHex8(dirEntry[i].attributes);
    delete fname;

    uint32_t firstFileCluster = ((uint32_t) dirEntry[i].firstClusterHi) << 16 | ((uint32_t) dirEntry[i].firstClusterLow);

    if((dirEntry[i].attributes & 0x10) == 0x10) { // Directory
      int32_t dsize = dirEntry[i].size;
      printf(" dir size: ");
      printHex8(dsize);
      printf("\n");
      if(isRoot || i >= 2) {
        uint32_t fileSector = dataStart + bpb.sectorsPerCluster * (firstFileCluster-2);
        tmpReadDir(ata, bpb, fatStart, dataStart, fileSector, false, dirDepth + 1);
      }
      continue;
    }

    int32_t size = dirEntry[i].size;
    if(size == 0) {
      size = 512;
    }

    uint32_t nextFileCluster = firstFileCluster;

    printf(" content: ");
    while(size > 0) {
      uint32_t fileSector = dataStart + bpb.sectorsPerCluster * (nextFileCluster-2);
      uint32_t sectorOffset = 0;
      while(size > 0) {
        ata->read(fileSector + sectorOffset, buffer, 512);
        buffer[dirEntry[i].size > 512 ? 512 : size] = 0;
        printf((char*) buffer);

        size -= 512;

        if(++sectorOffset > bpb.sectorsPerCluster) {
          break;
        }
      }

      uint32_t sectorCurrentCluster = nextFileCluster / (512/sizeof(uint32_t));
      ata->read(fatStart + sectorCurrentCluster, fatBuffer, 512);
      uint32_t sectorCurrentClusterOffset = nextFileCluster % (512/sizeof(uint32_t));
      nextFileCluster = ((uint32_t*) fatBuffer)[sectorCurrentClusterOffset] & 0x0FFFFFFF;
    }
    printf("\n");
  }
}

void Fat::readBPB(partition::Partition* partition) {
  fatBPB bpb;

  partition->read(0, (uint8_t*) &bpb, sizeof(fatBPB));

  uint32_t fatStart = bpb.reservedSectors;
  uint32_t tableSize = bpb.tableSize;
  uint32_t dataStart = fatStart + (tableSize * bpb.fatCopies);

  uint32_t rootStart = dataStart + bpb.sectorsPerCluster*bpb.rootCluster - 2;

  tmpReadDir(partition, bpb, fatStart, dataStart, rootStart);
  printHex32(bpb.byesPerSector);
  Fat* fat = new Fat(partition);
  delete fat;
}

bool Fat::isFat(Partition* partition) {
  fatBPB bpb;

  partition->read(0, (uint8_t*) &bpb, sizeof(fatBPB));

  if(bpb.signature != 29 && bpb.signature != 28) {
    return false;
  }
  String* typeLabel = new String((char*) bpb.fatTypeLabel, 8);
  bool hasFatLabel = typeLabel->contains("FAT32");
  delete typeLabel;

  return hasFatLabel;
}
