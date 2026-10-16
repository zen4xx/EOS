# ===========================================================================
# EOS build (64-bit)
#
#   make            -> eos.img   (a full 1.44 MB floppy image, dd-able)
#   make run        -> boot it in QEMU as a floppy
#   make run-ahci   -> boot with AHCI disk controller
#   make clean
#
# Layout of eos.img:
#   LBA 0        stage 1 boot sector   (512 B, has a BPB)
#   LBA 1..4     stage 2 loader        (2 KiB)
#   LBA 5..      kernel.bin            (loaded to 0x1000, then jumps to 64-bit)
# ===========================================================================

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c syscall/*.c multitasking/*.c)
HEADERS   = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h syscall/*.h multitasking/*.h)
OBJ       = $(C_SOURCES:.c=.o) cpu/interrupts.o multitasking/mt_asm.o cpu/gdt_flush.o

# 64-bit cross-compiler toolchain
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
GDB     = x86_64-elf-gdb

# 64-bit specific CFLAGS
CFLAGS  = -m64 -g -Wall -ffreestanding -nostdlib -fno-builtin \
          -fno-pie -fno-pic -fno-stack-protector \
          -fno-asynchronous-unwind-tables \
          -mcmodel=small -mno-red-zone -mno-mmx -mno-sse -mno-sse2

# Linker flags for 64-bit ELF
LDFLAGS = -m elf_x86_64 -z noexecstack -z separate-code -T boot/link.ld -nostdlib

FLOPPY_BYTES = 1474560

.PHONY: all run run-usb run-hdd run-ahci debug clean
all: eos.img

eos.img: boot/stage1.bin boot/stage2.bin kernel.bin
	cat boot/stage1.bin boot/stage2.bin kernel.bin > $@
	@truncate -s $(FLOPPY_BYTES) $@
	@echo "eos.img ready ($(FLOPPY_BYTES) bytes)"

boot/stage1.bin: boot/stage1.asm
	nasm $< -f bin -o $@

boot/stage2.bin: boot/stage2.asm kernel.bin
	@SZ=$$(stat -c%s kernel.bin); SECT=$$(( ($$SZ + 511) / 512 )); \
	 echo "kernel.bin = $$SZ bytes -> $$SECT sectors"; \
	 if [ $$SECT -gt 52 ]; then \
	   echo "kernel.bin is $$SZ bytes; it loads at 0x1000 and would run into"; \
	   echo "stage 2's real-mode stack at 0x7C00. Keep it under 26624 bytes,"; \
	   echo "or move the load address / stack in boot/stage2.asm."; \
	   exit 1; \
	 fi; \
	 nasm $< -f bin -D KERNEL_SECTORS=$$SECT -o $@

kernel.elf: boot/kernel_entry.o $(OBJ) boot/link.ld
	$(LD) $(LDFLAGS) -o $@ boot/kernel_entry.o $(OBJ)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm $< -f elf64 -o $@

# --- QEMU ------------------------------------------------------------------

# Floppy boot (original)
run: eos.img
	qemu-system-x86_64 -drive file=eos.img,format=raw,if=floppy -boot a

# USB mass storage emulation
run-usb: eos.img
	qemu-system-x86_64 -drive if=none,id=stick,format=raw,file=eos.img \
	                   -usb -device usb-storage,drive=stick -boot c

# Legacy IDE hard disk
run-hdd: eos.img
	qemu-system-x86_64 -drive file=eos.img,format=raw,if=ide,index=0 -boot c

# AHCI (SATA) controller
# Boot from floppy, attach eos.img also as SATA disk for AHCI driver testing
run-ahci: eos.img
	qemu-system-x86_64 \
		-drive file=eos.img,format=raw,if=none,id=boot \
		-device ahci,id=ahci \
		-device ide-hd,drive=boot,bus=ahci.0 \
		-boot c -m 512

debug: eos.img kernel.elf
	qemu-system-x86_64 -s -S \
		-drive file=eos.img,format=raw,if=none,id=boot \
		-device ahci,id=ahci \
		-device ide-hd,drive=boot,bus=ahci.0 \
		-boot c -m 512 &
	$(GDB) -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

clean:
	rm -f eos.img os-image.bin kernel.bin kernel.elf
	rm -f boot/*.bin boot/*.o kernel/*.o drivers/*.o cpu/*.o libc/*.o \
	      syscall/*.o multitasking/*.o