#include <cli/cli.h>
#include <cli/ping.h>
#include <cli/traceroute.h>
#include <cli/tcp.h>
#include <cli/sd.h>
#include <cli/wget.h>
#include <cli/file.h>
#include <cli/audio.h>
#include <cli/waudio.h>
#include <cli/snake.h>
#include <memorymanagement/memorymanagement.h>
#include <timer.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;

void printf(const char*);
void printHex32(uint32_t);
void printHex8(uint8_t);


filesystem::File* Cmd::workingDirectory = 0;

Cmd::Cmd() {
  this->inputEnabled = true;
}
Cmd::~Cmd() {}
void Cmd::run(common::String** args, common::uint32_t argsLength) {}
void Cmd::onInput(common::String* input) {}
void Cmd::onKeyDown(char c) {}


Cli::Cli() : KeyboardEventHandler() {
  this->cmdBufferSize = 512;
  this->cmdBufferPos = 0;
  this->cmdBuffer = (char*) MemoryManager::activeMemoryManager->malloc(sizeof(char) * (this->cmdBufferSize + 1));
  this->cmd = 0;
  this->command = 0;
  this->running = true;
}

Cli::~Cli() {
  delete this->cmdBuffer;
}

void Cli::onKeyDown(char c) {
  if (this->cmd != 0 && !this->cmd->inputEnabled) {
    this->cmd->onKeyDown(c);
    return;
  }
  if (this->cmd != 0) {
    this->cmd->onKeyDown(c);
  }
  char* charToPrint = " ";
  if(c == '\n') { // New line
    this->cmdBuffer[this->cmdBufferPos] = 0;
    if(this->command != 0 && this->cmd == 0) {
      return;
    }
    if(this->cmd != 0) {
      String* tmp = new String(this->cmdBuffer);
      this->cmd->onInput(tmp);
      delete tmp;
    } else {
      this->command = new String(this->cmdBuffer);
    }
    this->cmdBufferPos = 0;
  } else if(c == '\b') { // Backspace
    if(this->cmdBufferPos != 0) {
      charToPrint[0] = '\b';
      printf(charToPrint);
      this->cmdBufferPos--;
    }
  } else { // Normal char
    if(this->cmdBufferPos < this->cmdBufferSize) {
      charToPrint[0] = c;
      printf(charToPrint);
      this->cmdBuffer[this->cmdBufferPos] = c;
      this->cmdBufferPos++;
    } else {
      printf("Too many chars!\n");
      printf("> ");
      this->cmdBufferPos = 0;
    }
  }
}

void Cli::run() {
  this->showCommandInput();
  while(this->running) {
    if(this->command != 0) {
      String* read = this->command;
      String* command = read->split(' ', 0);
      Cmd* cmd = 0;

      if(command->equals("tcp")) {
        cmd = new CmdTCP();
      } else if(command->equals("wget")) {
        cmd = new CmdWGET();
      } else if(command->equals("ping")) {
        cmd = new CmdPing();
      } else if(command->equals("traceroute")) {
        cmd = new CmdTraceRoute();
      } else if(command->equals("sd")) {
        cmd = new CmdSD();
      } else if(command->equals("file")) {
        cmd = new CmdFILE();
      } else if(command->equals("audio")) {
        cmd = new CmdAUDIO();
      } else if(command->equals("waudio")) {
        cmd = new CmdWAUDIO();
      } else if(command->equals("snake")) {
        cmd = new CmdSnake();
      } else { // Only show text
        if(command->equals("help")) {
          printf("--- HELP ---\n");
          printf("help: Shows this\n");
          printf("tcp <host> <port>: Send and receive text over TCP\n");
          printf("ping <host> <times>: Pings a host\n");
          printf("traceroute <host>: Traces the route to a host\n");
          printf("sd: Shows commands for Storage Devices\n");
          printf("file: Shows commands for files\n");
          printf("wget <host> <query>: Downloads a file\n");
          printf("audio <filename>: Plays a wave file\n");
          printf("waudio <host> <query>: Skip wget and play an audio file directly over http\n");
          printf("snake: Play snake\n");
        } else {
          printf("Invalid command!\n");
        }
        this->showCommandInput();
        this->command = 0;
        delete command;
        delete read;
        continue;
      }

      this->cmd = cmd;

      uint32_t argsCount = read->occurrences(' ');
      String** args = (String**) MemoryManager::activeMemoryManager->malloc(sizeof(String*) * argsCount);
      for(int i = 0; i < argsCount; i++) {
        args[i] = read->split(' ', i + 1);
      }
      printf("\n");
      cmd->run(args, argsCount);

      this->cmd = 0;
      this->command = 0;

      for(int i = 0; i < argsCount; i++) {
        delete args[i];
      }
      delete args;
      delete command;
      delete read;
      delete cmd;
      this->showCommandInput();
    }
    asm("hlt");
  }
}

void Cli::showCommandInput() {
  printf("\n");
  if(Cmd::workingDirectory != 0) {
    String* filename = Cmd::workingDirectory->getFilename()->trim(' ');
    printf(filename->getCharPtr());
    delete filename;
  }
  printf(">");
}
