# MyOS

A small kernel with networking and disk IO capabilities. This is a side project.

## Getting Started

It is best to always clean before building, this prevents weird bugs that arise from alreay existing object files.

To cleanup execute the following command: 
  make clean

To build execute the following command: 
  make export/mykernel.iso

### Prerequisites

sudo apt-get install g++ binutils libc6-dev-i386

sudo apt-get install grub-pc-bin

sudo apt-get install xorriso
