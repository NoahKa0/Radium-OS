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


CmdFILE::CmdFILE() {}
CmdFILE::~CmdFILE() {}

void CmdFILE::run(String** args, uint32_t argsLength) {
  if(argsLength < 1) {
    printf("--- HELP ---\n");
    printf("cfs <device> <partition>: Change working directory to filesystem root\n");
    printf("cd <folder>: Change working directory this folder\n");
    printf("ls: Shows all files in current working directory\n");
    printf("cat <file>: Print contents of the file\n");
    printf("append <file> <data>, ...: Appends all arguments after the filename to the file\n");
    printf("delete <file>: Deletes a file\n");
    printf("mkdir <name>: Creates a new directory\n");
    printf("mkfile <name>: Creates a new empty file\n");
    return;
  }

}
