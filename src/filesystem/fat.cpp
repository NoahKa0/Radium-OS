#include <filesystem/fat.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::filesystem::partition;

void printf(char*);
void printHex8(uint8_t);
void printHex32(uint32_t);

FatFile::FatFile(Fat* fat, bool isFolder, uint32_t firstFileCluster, uint32_t parentCluster, String* filename, uint32_t size)
:File(fat)
{
  this->fat = fat;
  this->directory = isFolder;
  this->firstFileCluster = firstFileCluster;
  this->nextFileCluster = firstFileCluster;
  this->parentCluster = parentCluster;
  this->lastCluster = 0;
  this->size = isFolder ? 0 : size;
  this->readPosition = 0;
  this->sectorOffset = 0;
  this->filename = filename;
  this->buffer = (uint8_t*) MemoryManager::activeMemoryManager->mallocalign(512, 4);

  this->reset();
}
FatFile::~FatFile() {
  delete this->filename;
  delete buffer;
}

bool FatFile::updateChild(uint32_t childCluster, uint32_t size, String* name) {
  if(!this->directory) {
    return false;
  }
  if(this->nextFileCluster != this->firstFileCluster) {
    this->reset();
  }
  fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
  uint32_t total = 0;

  for(int i = 0; entry[i].name[0] != 0; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    if(entry[i].name[0] == 0xE5 || (entry[i].attributes & 0x0F) == 0x0F) { // Ignored
      continue;
    }
    uint32_t fileCluster = ((uint32_t) entry[i].firstClusterHi) << 16 | ((uint32_t) entry[i].firstClusterLow);
    if(childCluster == fileCluster) {
      entry[i].size = size;
      if(name != 0 && name->getLength() == 11) {
        for(int n = 0; n < 11; n++) {
          entry[i].name[n] = name->charAt(n);
        }
      }
      this->writeBuffer();
      return true;
    }
  }
  return false;
}

bool FatFile::deleteChild(uint32_t childCluster) {
  if(!this->directory) {
    return false;
  }
  if(this->nextFileCluster != this->firstFileCluster) {
    this->reset();
  }
  fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
  uint32_t total = 0;
  uint32_t entryId = 0;

  for(int i = 0; entry[i].name[0] != 0; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    if(entry[i].name[0] == 0xE5 || (entry[i].attributes & 0x0F) == 0x0F) { // Ignored
      total++;
      continue;
    }
    uint32_t fileCluster = ((uint32_t) entry[i].firstClusterHi) << 16 | ((uint32_t) entry[i].firstClusterLow);
    if(childCluster == fileCluster) {
      entryId = total;
    }
    total++;
  }

  uint8_t* fdata = (uint8_t*) MemoryManager::activeMemoryManager->malloc(total * sizeof(fatDirectoryEntry) + 512);
  this->reset();
  int f = 0;
  for(int i = 0; entry[i].name[0] != 0; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    for(int x = 0; x < sizeof(fatDirectoryEntry); x++) {
      fdata[f] = this->buffer[i * sizeof(fatDirectoryEntry) + x];
      f++;
    }
  }

  fatDirectoryEntry* entries = (fatDirectoryEntry*) entries;
  for(int i = 0; entries[i].name[0] != 0; i++) {
    printf(".");
    if(i >= entryId) {
      for(int e = 0; e < sizeof(fatDirectoryEntry); e++) {
        fdata[i + e] = fdata[(i * sizeof(fatDirectoryEntry)) + e];
      }
    }
  }

  uint32_t length = (total + 1) * sizeof(fatDirectoryEntry);
  if(length + sizeof(fatDirectoryEntry) > 512) {
    this->reset();
    this->loadNextSector();
    this->fat->deleteChain(this->nextFileCluster);
    this->fat->chain(this->firstFileCluster, true);
  }
  this->reset();

  printf("Do we even reach here???");

  uint32_t write = 0;
  for(int i = 0; i < length; i++) {
    if(write >= 512) {
      this->writeBuffer();
      if(this->fat->chain(this->nextFileCluster) == 0) {
        return false;
      }
      this->loadNextSector();
      write = 0;
    }
    this->buffer[write] = fdata[i];

    write++;
  }
  this->writeBuffer();
  delete fdata;
}

String* FatFile::getRealFilename(String* filename) {
  String* realName = new String("           ");
  char* chars = realName->getCharPtr();
  if(filename->occurrences('.') != 0 || filename->getLength() > 3) {
    for(int i = 0; i < filename->getLength() - 3 && i < realName->getLength(); i++) {
      if(filename->charAt(i) != '.') {
        chars[i] = filename->charAt(i);
      }
    }
    for(int i = 1; i < 4; i++) {
      chars[11-i] = filename->charAt(filename->getLength() - i);
    }
  } else {
    for(int i = 0; i < filename->getLength() && i < realName->getLength(); i++) {
      chars[i] = filename->charAt(i);
    }
  }
  realName->toUpper();
  return realName;
}

void FatFile::reset() {
  this->nextFileCluster = firstFileCluster;
  this->readPosition = 0;
  this->sectorOffset = 0;
  this->loadNextSector();
}

void FatFile::loadNextSector() {
  if(sectorOffset >= fat->sectorsPerCluster) {
    uint32_t sectorCurrentCluster = nextFileCluster / (512/sizeof(uint32_t));
    fat->partition->read(fat->fatStart + sectorCurrentCluster, this->buffer, 512);
    uint32_t sectorCurrentClusterOffset = nextFileCluster % (512/sizeof(uint32_t));
    this->nextFileCluster = ((uint32_t*) this->buffer)[sectorCurrentClusterOffset] & 0x0FFFFFFF;

    this->sectorOffset = 0;
  }

  uint32_t fileSector = fat->dataStart + fat->sectorsPerCluster * (nextFileCluster-2);

  this->fat->partition->read(fileSector + sectorOffset, this->buffer, 512);
  this->sectorOffset++;
}

void FatFile::writeBuffer() {
  this->sectorOffset--;
  uint32_t fileSector = fat->dataStart + fat->sectorsPerCluster * (nextFileCluster-2);
  this->fat->partition->write(fileSector + sectorOffset, this->buffer, 512);
  this->sectorOffset++;
}

bool FatFile::isFolder() {
  return this->directory;
}
File* FatFile::getParent() {
  if(this->directory) {
    if(this->nextFileCluster != this->firstFileCluster) {
      this->reset();
    }
    fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
    if(entry[1].name[0] != '.' || entry[1].name[1] != '.') {
      return 0;
    }
    uint32_t firstFileCluster = ((uint32_t) entry[1].firstClusterHi) << 16 | ((uint32_t) entry[1].firstClusterLow);
    return new FatFile(this->fat, true, firstFileCluster, 0, new String((char*) entry[1].name, 11));
  } else {
    return new FatFile(this->fat, true, this->parentCluster, 0, 0);
  }
  return 0;
}
File* FatFile::getChild(uint32_t file) {
  if(!this->directory) {
    return 0;
  }
  if(this->nextFileCluster != this->firstFileCluster) {
    this->reset();
  }
  fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
  uint32_t total = 0;

  for(int i = 0; entry[i].name[0] != 0; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    if(entry[i].name[0] == 0xE5 || (entry[i].attributes & 0x0F) == 0x0F) { // Ignored
      continue;
    }
    if(total == file) {
      bool directory = (entry[i].attributes & 0x10) == 0x10;
      uint32_t firstFileCluster = ((uint32_t) entry[i].firstClusterHi) << 16 | ((uint32_t) entry[i].firstClusterLow);
      return new FatFile(this->fat, directory, firstFileCluster, this->firstFileCluster, new String((char*) entry[i].name, 11), entry[i].size);
    }
    total++;
  }
  return 0;
}
File* FatFile::getChildByName(common::String* file) {
  if(!this->directory) {
    return 0;
  }
  String* realName = this->getRealFilename(file);

  if(this->nextFileCluster != this->firstFileCluster) {
    this->reset();
  }
  fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
  
  for(int i = 0; entry[i].name[0] != 0; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    if(entry[i].name[0] == 0xE5 || (entry[i].attributes & 0x0F) == 0x0F) { // Ignored
      continue;
    }
    if(realName->contains((char*) entry[i].name, 11)) {
      delete realName;

      bool directory = (entry[i].attributes & 0x10) == 0x10;
      uint32_t firstFileCluster = ((uint32_t) entry[i].firstClusterHi) << 16 | ((uint32_t) entry[i].firstClusterLow);
      return new FatFile(this->fat, directory, firstFileCluster, this->firstFileCluster, new String((char*) entry[i].name, 11), entry[i].size);
    }
  }
  delete realName;
  return 0;
}
uint32_t FatFile::numChildren() {
  if(!this->directory) {
    return 0;
  }
  if(this->size != 0) {
    return this->size;
  }
  if(this->nextFileCluster != this->firstFileCluster) {
    this->reset();
  }
  fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
  uint32_t total = 0;

  for(int i = 0; entry[i].name[0] != 0; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    if(entry[i].name[0] == 0xE5 || (entry[i].attributes & 0x0F) == 0x0F) { // Ignored
      continue;
    }
    total++;
  }
  this->size = total;
  return total;
}
bool FatFile::mkFile(String* filename, bool folder) {
  if(!this->directory) {
    return 0;
  }
}

bool FatFile::append(uint8_t* data, uint32_t length) {
  if(this->directory) {
    return false;
  }
  while(this->hasNext() > 512) {
    this->readPosition += 512;
    this->loadNextSector();
  }
  while(this->hasNext() > 0) {
    this->nextByte();
  }
  uint32_t write = this->size % 512;
  for(int i = 0; i < length; i++) {
    if(write >= 512) {
      this->writeBuffer();
      if(this->fat->chain(this->nextFileCluster) == 0) {
        return false;
      }
      this->loadNextSector();
      write = 0;
    }
    this->buffer[write] = data[i];

    write++;
    this->size++;
    this->readPosition++;
  }
  this->writeBuffer();

  FatFile* parent = (FatFile*) this->getParent();
  if(parent != 0) {
    parent->updateChild(this->firstFileCluster, this->size);
    delete parent;
    return true;
  }
  return false;
}

int FatFile::hasNext() {
  return this->size - this->readPosition;
}

uint8_t FatFile::nextByte() {
  if(this->directory || this->hasNext() <= 0) {
    return 0;
  }
  uint8_t ret = this->buffer[this->readPosition % 512];
  this->readPosition++;
  if(this->readPosition % 512 == 0) {
    this->loadNextSector();
  }
  return ret;
}

void FatFile::read(uint8_t* buffer, uint32_t length) {
  if(this->directory) {
    return;
  }
  if(length > this->hasNext()) {
    length = this->hasNext();
  }
  for(uint32_t i = 0; i < length; i++) {
    buffer[i] = this->nextByte();
  }
}

String* FatFile::getFilename() {
  return this->filename;
}

bool FatFile::remove() {
  if(this->directory && this->numChildren() > 2) {
    return false;
  }
  FatFile* parent = (FatFile*) this->getParent();
  if(parent == 0) {
    return false;
  }
  parent->deleteChild(this->firstFileCluster);
  delete parent;
  this->fat->deleteChain(this->firstFileCluster);
  return true;
}

bool FatFile::rename(String* name) {
  FatFile* parent = (FatFile*) this->getParent();
  if(parent == 0) {
    return false;
  }
  if(this->filename != 0) {
    delete this->filename;
  }
  this->filename = this->getRealFilename(name);
  bool ret = parent->updateChild(this->firstFileCluster, this->size, this->filename);
  delete parent;
  return ret;
}

void recursivePrintFile(File* file, int depth) {
  for(int i = 0; i < file->numChildren(); i++) {
    File* tmp = file->getChild(i);
    for(int i = 0; i < depth; i++) printf("  ");
    printf("File: ");
    printf(tmp->getFilename()->getCharPtr());
    if(tmp->isFolder()) {
      printf("  dir\n");
      if(tmp->getFilename()->charAt(0) != '.') {
        recursivePrintFile(tmp, depth + 1);
      }
    } else {
      printf("  content: ");
      uint8_t* bytes = (uint8_t*) MemoryManager::activeMemoryManager->malloc(tmp->hasNext() + 1);
      bytes[tmp->hasNext()] = 0;
      tmp->read(bytes, tmp->hasNext());
      printf((char*) bytes);
      delete bytes;
      printf("\n");
    }
    delete tmp;
  }
}


Fat::Fat(Partition* partition) {
  this->bpb = (fatBPB*) MemoryManager::activeMemoryManager->malloc(sizeof(fatBPB) > 512 ? sizeof(fatBPB) : 512);
  partition->read(0, (uint8_t*) this->bpb, 512);
  this->info = (FATInfo*) MemoryManager::activeMemoryManager->malloc(sizeof(FATInfo) > 512 ? sizeof(FATInfo) : 512);
  partition->read(this->bpb->fatInfo, (uint8_t*) this->info, 512);
  this->partition = partition;

  this->fatStart = bpb->reservedSectors;
  this->dataStart = this->fatStart + (bpb->tableSize * bpb->fatCopies);
  this->rootStart = this->dataStart + bpb->sectorsPerCluster*bpb->rootCluster - 2;
  this->sectorsPerCluster = bpb->sectorsPerCluster;

  File* file = this->getRoot();
  recursivePrintFile(file, 0);

  printf("Writing to RADIUM.TXT\n");
  String* data = new String("Radon.txt");
  String* fname = new String("Radium.txt");
  File* radium = file->getChildByName(fname);
  if(radium != 0) {
    bool result = radium->remove();
    if(result) {
      printf("SUCCESS\n");
    } else {
      printf("FAILED\n");
    }
    delete radium;
  } else {
    printf("ERROR: Radium.txt not found!\n");
  }
  delete data;
  delete fname;
  delete file;
}
Fat::~Fat() {
  if(this->info != 0) {
    this->partition->write(this->bpb->fatInfo, (uint8_t*) this->info, 512);
    delete this->info;
  }
  if(this->bpb != 0) {
    delete this->bpb;
  }
  this->bpb = 0;
  this->partition = 0;
}

uint32_t Fat::chain(uint32_t lastCluster, bool force) {
  uint32_t search = this->info->startAvailableClusters;
  if(search == 0xFFFFFFFF) {
    search = 2;
  }
  uint32_t buffer[128];
  uint32_t result = 1;

  uint32_t sectorCurrentCluster = lastCluster / (512/sizeof(uint32_t));
  this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
  uint32_t sectorCurrentClusterOffset = lastCluster % (512/sizeof(uint32_t));
  uint32_t clusterRead = buffer[sectorCurrentClusterOffset] & 0x0FFFFFFF;
  if(clusterRead < 0x0FFFFFF8 && !force) {
    return clusterRead;
  }

  uint32_t sectorLastCluster = 0xFFFFFFFF;
  uint32_t totalClusters = this->bpb->tableSize / 4;
  while(result != 0 && search < totalClusters) {
    search++;
    sectorCurrentCluster = search / (512/sizeof(uint32_t));
    if(sectorCurrentCluster != sectorLastCluster) {
      sectorLastCluster = sectorCurrentCluster;
      this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
    }
    sectorCurrentClusterOffset = search % (512/sizeof(uint32_t));
    result = buffer[sectorCurrentClusterOffset] & 0x0FFFFFFF;
  }
  if(result != 0) {
    return 0;
  }
  buffer[sectorCurrentClusterOffset] |= 0x0FFFFFF8;
  this->partition->write(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
  
  sectorCurrentCluster = lastCluster / (512/sizeof(uint32_t));
  this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
  sectorCurrentClusterOffset = lastCluster % (512/sizeof(uint32_t));
  buffer[sectorCurrentClusterOffset] &= 0xF0000000;
  buffer[sectorCurrentClusterOffset] |= search;
  this->partition->write(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);

  this->info->startAvailableClusters = search;
  if(this->info->freeClusterCount != 0xFFFFFFFF) {
    this->info->freeClusterCount--;
  }
  return search;
}

bool Fat::deleteChain(uint32_t startCluster) {
  uint32_t cluster = startCluster;
  uint32_t loadedSector = cluster / (512/sizeof(uint32_t));
  uint32_t sectorCurrentCluster;
  uint32_t sectorCurrentClusterOffset;
  uint32_t buffer[128];
  this->partition->read(this->fatStart + loadedSector, (uint8_t*) &buffer[0], 512);
  while(cluster >= 0x0FFFFFF8 && cluster != 0) {
    sectorCurrentCluster = cluster / (512/sizeof(uint32_t));
    if(sectorCurrentCluster != loadedSector) {
      this->partition->write(this->fatStart + loadedSector, (uint8_t*) &buffer[0], 512);
      this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
      loadedSector = sectorCurrentCluster;
    }
    sectorCurrentClusterOffset = cluster % (512/sizeof(uint32_t));
    cluster = buffer[sectorCurrentClusterOffset] & 0x0FFFFFFF;
    buffer[sectorCurrentClusterOffset] = 0;
    if(this->info->freeClusterCount != 0xFFFFFFFF) {
      this->info->freeClusterCount--;
    }
  }
  this->partition->write(this->fatStart + loadedSector, (uint8_t*) &buffer[0], 512);
  return true;
}

File* Fat::getRoot() {
  return new FatFile(this, true, this->bpb->rootCluster, 0, new String(), 0);
}
String* Fat::getName() {
  return new String();
}
String* Fat::getType() {
  return new String("FAT32");
}

bool Fat::isFat(Partition* partition) {
  fatBPB bpb;

  partition->read(0, (uint8_t*) &bpb, sizeof(fatBPB));

  printf("Typelabel: ");
  String* typeLabel = new String((char*) bpb.fatTypeLabel, 8);
  printf(typeLabel->getCharPtr());
  bool hasFatLabel = typeLabel->contains("FAT32");
  delete typeLabel;

  if(bpb.signature != 0x29 && bpb.signature != 0x28) {
    printf("Signature: ");
    printHex32(bpb.signature);
    return false;
  }

  return hasFatLabel;
}
