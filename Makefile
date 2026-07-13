AS = nasm
CC = gcc
LD = gcc

ASFLAGS = -f elf32
CFLAGS = -m32 -c -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -fno-pic
LDFLAGS = -m32 -T linker.ld -ffreestanding -O2 -nostdlib -fno-pie -fno-stack-protector -fno-pic

all: myos.bin

myos.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) -o myos.bin boot.o kernel.o -lgcc

boot.o: boot.s
	$(AS) $(ASFLAGS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

run: myos.bin
	qemu-system-i386 -kernel myos.bin

clean:
	rm -f *.o myos.bin
write:
	rm -f iso/boot/myos.bin
	cp myos.bin iso/boot/
	rm -f build.iso
	grub-mkrescue -o build.iso iso
	#sudo dd if=build.iso of=/dev/sdb bs=4M status=progress && sync
create:
	rm -f iso/boot/myos.bin
	cp myos.bin iso/boot/
	rm -f build.iso
	grub-mkrescue -o build.iso iso
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential nasm gcc-multilib grub-pc-bin xorriso mtools