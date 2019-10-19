GCCPARAMS = -m32 -Iinclude -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
objects = tmp/loader.o \
          tmp/gdt.o \
          tmp/memorymanagement.o \
          tmp/timer.o \
          tmp/drivers/driver.o \
          tmp/hardware/port.o \
          tmp/hardware/interruptstubs.o \
          tmp/hardware/interrupts.o \
          tmp/hardware/pci.o \
          tmp/drivers/keyboard.o \
          tmp/drivers/mouse.o \
          tmp/drivers/vga.o \
          tmp/drivers/ata.o \
          tmp/drivers/ethernet_driver.o \
          tmp/drivers/amd_am79c973.o \
          tmp/net/etherframe.o \
          tmp/net/arp.o \
          tmp/net/ipv4.o \
          tmp/net/icmp.o \
          tmp/net/udp.o \
          tmp/net/tcp.o \
          tmp/net/dhcp.o \
          tmp/systemcalls.o \
          tmp/multitasking.o \
          tmp/kernel.o

tmp/%.o: src/%.cpp
	mkdir -p $(@D)
	g++ $(GCCPARAMS) -o $@ -c $<

tmp/%.o: src/%.s
	mkdir -p $(@D)
	as --32 -o $@ $<

export/mykernel.bin: linker.ld $(objects)
	ld -melf_i386 -T $< -o $@ $(objects)

export/mykernel.iso: export/mykernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp export/mykernel.bin iso/boot/mykernel.bin
	echo 'set timeout=0'                      > iso/boot/grub/grub.cfg
	echo 'set default=0'                     >> iso/boot/grub/grub.cfg
	echo ''                                  >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin'    >> iso/boot/grub/grub.cfg
	echo '  boot'                            >> iso/boot/grub/grub.cfg
	echo '}'                                 >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=export/mykernel.iso iso
	rm -rf iso
	
.PHONY: clean
clean:
	rm -rf tmp export
	mkdir export
	
