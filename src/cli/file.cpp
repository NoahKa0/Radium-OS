#include <cli/file.h>
#include <drivers/storage/storageDevice.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;
using namespace sys::filesystem;
using namespace sys::filesystem::partition;

void printf(char*);
void printHex32(uint32_t);
void printHex8(uint8_t);


CmdFILE::CmdFILE() {
  this->appendFile = 0;
}
CmdFILE::~CmdFILE() {}

void CmdFILE::run(String** args, uint32_t argsLength) {
  if(argsLength < 1) {
    printf("--- HELP ---\n");
    printf("cfs <device> <partition>: Change working directory to filesystem root\n");
    printf("cd <folder>: Change working directory this folder\n");
    printf("ls: Shows all files in current working directory\n");
    printf("cat <file>: Print contents of the file\n");
    printf("append <file>: Appends a file. Press enter twice to quit.\n");
    printf("delete <file>: Deletes a file\n");
    printf("mkdir <name>: Creates a new directory\n");
    printf("mkfile <name>: Creates a new empty file\n");
    return;
  }

  if(args[0]->equals("cd") && argsLength >= 2) {
    if(this->workingDirectory != 0) {
      String* filename = new String();
      for(int i = 1; i < argsLength; i++) {
        filename->append(args[i]->getCharPtr());
        if(i+1 != argsLength) {
          filename->append(" ");
        }
      }
      File* tmp = this->workingDirectory->getChildByName(filename);
      if(tmp != 0) {
        this->changeWorkingDirectory(tmp);
      } else {
        printf("File not found!\n");
      }
      delete filename;
    } else {
      printf("Working directory is not set!\n");
    }
  } else if(args[0]->equals("cfs") && argsLength >= 3) {
    uint32_t deviceId = args[1]->parseInt();
    uint32_t partitionId = args[2]->parseInt();
    if(deviceId >= 0 && partitionId >= 0) {
      Partition* partition = PartitionManager::activePartitionManager->getPartition(deviceId, partitionId);
      if(partition != 0 && partition->getFileSystem() != 0) {
        File* tmp = partition->getFileSystem()->getRoot();
        if(tmp != 0) {
          this->changeWorkingDirectory(tmp);
        } else {
          printf("Filesystem has no root!\n");
        }
      } else {
        printf("Invalid partition!\n");
      }
    } else {
      printf("Device or partition can not be below 0.\n");
    }
  } else if(args[0]->equals("ls")) {
    if(this->workingDirectory != 0) {
      for(int i = 0; i < this->workingDirectory->numChildren(); i++) {
        File* tmp = this->workingDirectory->getChild(i);
        if(tmp != 0) { // This is always true.
          printf(tmp->getFilename()->getCharPtr());
          printf(";  ");
          if(tmp->isFolder()) {
            printf("folder");
          } else {
            printf("size: 0x");
            printHex32(tmp->hasNext());
          }
          printf("\n");
          delete tmp;
        }
      }
    } else {
      printf("No working directory!\n");
    }
  } else if(args[0]->equals("cat") && argsLength >= 2) {
    if(this->workingDirectory != 0) {
      String* filename = new String();
      for(int i = 1; i < argsLength; i++) {
        filename->append(args[i]->getCharPtr());
        if(i+1 != argsLength) {
          filename->append(" ");
        }
      }
      File* tmp = this->workingDirectory->getChildByName(filename);
      if(tmp != 0) {
        uint32_t length = tmp->hasNext();
        uint8_t* content = (uint8_t*) MemoryManager::activeMemoryManager->malloc(length + 1);
        tmp->read(content, length);
        content[length] = 0;
        printf((char*) content);
        delete content;
      } else {
        printf("File not found!\n");
      }
      delete filename;
    } else {
      printf("No working directory!\n");
    }
  } else if(args[0]->equals("append") && argsLength >= 2) {
    if(this->workingDirectory != 0) {
      String* filename = new String();
      for(int i = 1; i < argsLength; i++) {
        filename->append(args[i]->getCharPtr());
        if(i+1 != argsLength) {
          filename->append(" ");
        }
      }
      File* tmp = this->workingDirectory->getChildByName(filename);
      if(tmp != 0) {
        this->appendFile = tmp;
        while(this->appendFile != 0) {
          asm("hlt");
        }
        this->workingDirectory->reset();
      } else {
        printf("File not found!\n");
      }
      delete filename;
    } else {
      printf("Working directory is not set!\n");
    }
  } else if((args[0]->equals("mkdir") || args[0]->equals("mkfile")) && argsLength >= 2) {
    if(this->workingDirectory != 0) {
      String* filename = new String();
      for(int i = 1; i < argsLength; i++) {
        filename->append(args[i]->getCharPtr());
        if(i+1 != argsLength) {
          filename->append(" ");
        }
      }
      bool result = this->workingDirectory->mkFile(filename, args[0]->equals("mkdir"));
      delete filename;
      if(!result) {
        printf("File already exists!");
      }
    } else {
      printf("Working directory is not set!\n");
    }
  } else if(args[0]->equals("delete") && argsLength >= 2) {
    if(this->workingDirectory != 0) {
      String* filename = new String();
      for(int i = 1; i < argsLength; i++) {
        filename->append(args[i]->getCharPtr());
        if(i+1 != argsLength) {
          filename->append(" ");
        }
      }
      File* tmp = this->workingDirectory->getChildByName(filename);
      if(tmp != 0) {
        tmp->remove();
        delete tmp;
        this->workingDirectory->reset();
      } else {
        printf("File not found!\n");
      }
      delete filename;
    } else {
      printf("Working directory is not set!\n");
    }
  } else {
    printf("Try without arguments for help!\n");
  }

}

void CmdFILE::onInput(common::String* input) {
  if(this->appendFile != 0) {
    printf("\n");
    input->append("\n");
    if(input->equals("\n")) {
      delete this->appendFile;
      this->appendFile = 0;
    } else {
      this->appendFile->append((uint8_t*) input->getCharPtr(), input->getLength());
    }
  }
}

void CmdFILE::changeWorkingDirectory(File* file) {
  if(this->workingDirectory != 0) {
    delete this->workingDirectory;
  }
  this->workingDirectory = file;
}
