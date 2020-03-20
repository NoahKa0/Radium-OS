#include <filesystem/fat.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::drivers;

void printf(char*);
void printHex8(uint8_t);


void tmpReadDir(AdvancedTechnologyAttachment* ata, fatBPB bpb, uint32_t fatStart, uint32_t dataStart, uint32_t dirSector, bool isRoot = true, uint8_t dirDepth = 0) {
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
      printf(" dir\n");
      // Microsoft actually added the folders . and .. didn't see that coming.
      if(isRoot || i >= 2) {
        uint32_t fileSector = dataStart + bpb.sectorsPerCluster * (firstFileCluster-2);
        tmpReadDir(ata, bpb, fatStart, dataStart, fileSector, false, dirDepth + 1);
      }
      continue;
    }

    uint32_t sectorOffset = 0;
    int32_t size = dirEntry[i].size;
    if(size == 0) {
      size = 512;
    }

    uint32_t nextFileCluster = firstFileCluster;

    printf(" content: ");
    while(size > 0) {
      uint32_t fileSector = dataStart + bpb.sectorsPerCluster * (nextFileCluster-2);
      while(size > 0) {
        ata->read(fileSector + sectorOffset, buffer, 512);
        buffer[dirEntry[i].size > 512 ? 512 : size] = 0;
        printf((char*) buffer);

        size -= 512;

        if(++sectorOffset > bpb.byesPerSector) {
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

void fat::readBPB(AdvancedTechnologyAttachment* ata, uint32_t offset) {
  fatBPB bpb;

  ata->read(offset, (uint8_t*) &bpb, sizeof(fatBPB));

  printf("\nFat\n");

  uint32_t fatStart = offset + bpb.reservedSectors;
  uint32_t tableSize = bpb.tableSize;
  uint32_t dataStart = fatStart + (tableSize * bpb.fatCopies);

  uint32_t rootStart = dataStart + bpb.sectorsPerCluster*bpb.rootCluster - 2;

  tmpReadDir(ata, bpb, fatStart, dataStart, rootStart);
}
