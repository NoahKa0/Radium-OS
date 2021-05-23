#include <filesystem/fat.h>

using namespace sys;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::filesystem::partition;

FatFile::FatFile(Fat* fat, bool isFolder, uint32_t firstFileCluster, uint32_t parentCluster, Array* path, uint32_t size, bool root)
:File(fat)
{
  this->root = root;
  this->fat = fat;
  this->directory = isFolder;
  this->firstFileCluster = firstFileCluster;
  this->nextFileCluster = firstFileCluster;
  this->parentCluster = parentCluster;
  this->lastCluster = 0;
  this->size = isFolder ? 0 : size;
  this->readPosition = 0;
  this->sectorOffset = 0;
  this->path = path;
  this->buffer = (uint8_t*) MemoryManager::activeMemoryManager->mallocalign(512, 4);

  this->reset();
}
FatFile::~FatFile() {
  delete this->path;
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
      if(i < 15 && entry[i + 1].name[0] == 0) {
        entry[i].name[0] = 0x00;
      } else {
        entry[i].name[0] = 0xE5;
      }
      this->writeBuffer();
      return true;
    }
  }
  return false;
}

String* FatFile::getRealFilename(String* filename) {
  String* realName = new String("           ");
  char* chars = realName->getCharPtr();
  if(filename->occurrences('.') != 0 && filename->getLength() > 3) {
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
  if(chars[0] == ' ') { // I want all name strings to have a Real name, even though this name should actually be considered illegal.
    chars[0] = 'A';
  }
  realName->toUpper();
  return realName;
}

void FatFile::reset() {
  if(this->directory) {
    this->size = 0;
  }
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

Array* FatFile::copyCurrentPath() {
  Array* ret = new Array(true);
  for(int i = 0; i < this->path->getLength(); i++) {
    ret->add((uint8_t*) new String(((String*) (this->path->get(i)))->getCharPtr()));
  }
  return ret;
}

bool FatFile::isFolder() {
  return this->directory;
}
File* FatFile::getParent() {
  Array* parentPath = this->copyCurrentPath();
  parentPath->remove(parentPath->getLength() - 1);
  if(this->directory) {
    if(this->nextFileCluster != this->firstFileCluster) {
      this->reset();
    }
    fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;
    if(entry[1].name[0] != '.' || entry[1].name[1] != '.') {
      delete parentPath;
      return 0;
    }
    uint32_t firstFileCluster = ((uint32_t) entry[1].firstClusterHi) << 16 | ((uint32_t) entry[1].firstClusterLow);
    if(firstFileCluster == 0) {
      delete parentPath;
      return this->fat->getRoot();
    }
    return new FatFile(this->fat, true, firstFileCluster, 0, parentPath);
  }
  return new FatFile(this->fat, true, this->parentCluster, 0, parentPath);
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
      if(firstFileCluster == 0) {
        return this->fat->getRoot();
      }
      Array* childPath = this->copyCurrentPath();
      childPath->add((uint8_t*) new String((char*) entry[i].name, 11));
      return new FatFile(this->fat, directory, firstFileCluster, this->firstFileCluster, childPath, entry[i].size);
    }
    total++;
  }
  return 0;
}
File* FatFile::getChildByName(String* file) {
  if(!this->directory) {
    return 0;
  }
  String* realName = this->getRealFilename(file);
  if(realName->equals(".          ") || realName->equals("..         ")) {
    File* parent = this->getParent();
    if(realName->getCharPtr()[1] == '.') {
      delete realName;
      return parent;
    } else {
      File* me = parent->getChildByName(this->getFilename());
      delete realName;
      delete parent;
      return me;
    }
  }

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
      if(firstFileCluster == 0) {
        return this->fat->getRoot();
      }
      Array* childPath = this->copyCurrentPath();
      childPath->add((uint8_t*) new String((char*) entry[i].name, 11));
      return new FatFile(this->fat, directory, firstFileCluster, this->firstFileCluster, childPath, entry[i].size);
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
    return false;
  }
  File* check = this->getChildByName(filename);
  if(check != 0) {
    delete check;
    return false;
  }
  delete check;

  uint32_t chainStart = this->fat->chain(0, true);
  String* realname = this->getRealFilename(filename);
  if(this->nextFileCluster != this->firstFileCluster) {
    this->reset();
  }
  fatDirectoryEntry* entry = (fatDirectoryEntry*) this->buffer;

  bool added = false;
  for(int i = 0; !added; i++) {
    if(i >= 16) {
      this->loadNextSector();
      i = 0;
    }
    if(entry[i].name[0] == 0xE5 || entry[i].name[0] == 0x00) {
      added = true;
      bool append = entry[i].name[0] == 0x00;
      for(int n = 0; n < 11; n++) {
        entry[i].name[n] = realname->charAt(n);
      }
      // 0x10 = folder, 0x20 = archive (for backups).
      entry[i].attributes = folder ? 0x10 : 0x20;
      entry[i].reserved = 0;

      entry[i].createdTimeTenth = 0;
      entry[i].createdTime = 0;
      entry[i].createdDate = 0;
      entry[i].accessedTime = 0;

      entry[i].writeTime = 0;
      entry[i].writeDate = 0;
      entry[i].size = 0;

      entry[i].firstClusterLow = (uint16_t) (chainStart & 0xFFFF);
      entry[i].firstClusterHi = (uint16_t) ((chainStart >> 16) & 0xFFFF);

      if(append) {
        if(i+1 < 16) {
          entry[i+1].name[0] = 0;
        } else {
          this->writeBuffer();
          this->fat->chain(this->nextFileCluster);
          this->loadNextSector();
          for(int b = 0; b < 512; b++) {
            this->buffer[b] = 0;
          }
        }
      }
      this->writeBuffer();
      if(folder) {
        for(int b = 0; b < 512; b++) {
          this->buffer[b] = 0;
        }
        for(int n = 0; n < 11; n++) {
          entry[0].name[n] = ' ';
          entry[1].name[n] = ' ';
        }
        entry[0].name[0] = '.';
        entry[1].name[0] = '.';
        entry[1].name[1] = '.';

        entry[0].attributes = 0x10;
        entry[1].attributes = 0x10;

        entry[0].firstClusterLow = (uint16_t) (chainStart & 0xFF);
        entry[0].firstClusterHi = (uint16_t) ((chainStart >> 16) & 0xFF);
        if(!this->root) {
          entry[1].firstClusterLow = (uint16_t) (this->firstFileCluster & 0xFF);
          entry[1].firstClusterHi = (uint16_t) ((this->firstFileCluster >> 16) & 0xFF);
        }

        uint32_t fileSector = fat->dataStart + fat->sectorsPerCluster * (chainStart-2);
        this->fat->partition->write(fileSector, this->buffer, 512);
        this->reset();
      }
      this->size = 0;
    }
  }

  delete realname;
  return true;
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
  return (String*) this->path->get(this->path->getLength() - 1);
}

bool FatFile::remove() {
  if(this->directory && this->numChildren() > 2) {
    for(int i = 0; i < this->numChildren(); i++) {
      File* tmp = this->getChild(i);
      if(tmp->getFilename()->charAt(0) != '.') {
        tmp->remove();
      }
      delete tmp;
    }
  }
  FatFile* parent = (FatFile*) this->getParent();
  if(parent == 0) {
    return false;
  }
  bool entryDeleted = parent->deleteChild(this->firstFileCluster);
  delete parent;
  if(entryDeleted) {
    this->fat->deleteChain(this->firstFileCluster);
  }
  return entryDeleted;
}

bool FatFile::rename(String* name) {
  FatFile* parent = (FatFile*) this->getParent();
  if(parent == 0) {
    return false;
  }
  this->path->set(this->path->getLength() - 1, (uint8_t*) this->getRealFilename(name));
  bool ret = parent->updateChild(this->firstFileCluster, this->size, this->getFilename());
  delete parent;
  return ret;
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

void Fat::writeFat(uint32_t sector, uint8_t* data) {
  for(int i = 0; i < this->bpb->fatCopies; i++) {
    this->partition->write(i * this->bpb->tableSize + this->fatStart + sector, data, 512);
  }
}

uint32_t Fat::chain(uint32_t lastCluster, bool force) {
  uint32_t search = this->info->startAvailableClusters;
  if(search == 0xFFFFFFFF) {
    search = 2;
  }
  uint32_t buffer[128];
  uint32_t result = 1;

  uint32_t sectorCurrentCluster = lastCluster / (512/sizeof(uint32_t));
  uint32_t sectorCurrentClusterOffset = lastCluster % (512/sizeof(uint32_t));
  if(lastCluster != 0 && !force) {
    this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
    uint32_t clusterRead = buffer[sectorCurrentClusterOffset] & 0x0FFFFFFF;
    if(clusterRead < 0x0FFFFFF8 && !force) {
      return clusterRead;
    }
  }

  uint32_t sectorLastCluster = 0xFFFFFFFF;
  uint32_t totalClusters = this->bpb->tableSize / 4;
  if (search >= totalClusters) {
    search = 2;
  }
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
  this->writeFat(sectorCurrentCluster, (uint8_t*) &buffer[0]);
  if(lastCluster == 0) {
    return search;
  }
  
  sectorCurrentCluster = lastCluster / (512/sizeof(uint32_t));
  this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
  sectorCurrentClusterOffset = lastCluster % (512/sizeof(uint32_t));
  buffer[sectorCurrentClusterOffset] &= 0xF0000000;
  buffer[sectorCurrentClusterOffset] |= search;
  this->writeFat(sectorCurrentCluster, (uint8_t*) &buffer[0]);

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
  uint32_t lowestRead = 0xFFFFFFFF;
  uint32_t buffer[128];
  this->partition->read(this->fatStart + loadedSector, (uint8_t*) &buffer[0], 512);
  while(cluster < 0x0FFFFFF8 && cluster != 0) {
    sectorCurrentCluster = cluster / (512/sizeof(uint32_t));
    if(sectorCurrentCluster != loadedSector) {
      this->writeFat(loadedSector, (uint8_t*) &buffer[0]);
      this->partition->read(this->fatStart + sectorCurrentCluster, (uint8_t*) &buffer[0], 512);
      loadedSector = sectorCurrentCluster;
    }
    sectorCurrentClusterOffset = cluster % (512/sizeof(uint32_t));
    cluster = buffer[sectorCurrentClusterOffset] & 0x0FFFFFFF;
    buffer[sectorCurrentClusterOffset] = 0;
    if(cluster < lowestRead) {
      lowestRead = cluster;
    }
    if(this->info->freeClusterCount != 0xFFFFFFFF) {
      this->info->freeClusterCount--;
    }
  }
  if(this->info->startAvailableClusters != 0xFFFFFFFF && this->info->startAvailableClusters > lowestRead) {
    this->info->startAvailableClusters = lowestRead;
  }
  this->writeFat(loadedSector, (uint8_t*) &buffer[0]);
  return true;
}

File* Fat::getRoot() {
  Array* path = new Array(true);
  path->add((uint8_t*) (new String("")));
  return new FatFile(this, true, this->bpb->rootCluster, 0, path, 0);
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

  if(bpb.signature != 0x29 && bpb.signature != 0x28) {
    return false;
  }
  
  String* typeLabel = new String((char*) bpb.fatTypeLabel, 8);
  bool hasFatLabel = typeLabel->contains("FAT32");
  delete typeLabel;

  return hasFatLabel;
}
