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

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c syscall/*.c multitasking/*.c fs/*.c)
HEADERS   = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h syscall/*.h multitasking/*.h fs/*.h)
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

EXT2_IMG = ext2_disk.img
EXT2_SIZE = 32

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
	 if [ $$SECT -gt 250 ]; then \
	   echo "kernel.bin is $$SZ bytes; it temporarily loads at 0x20000 and would run into"; \
	   echo "stage 2. Keep it under ~125 KB for the real-mode load phase, or increase KERNEL_TMP_OFF."; \
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

$(EXT2_IMG):
	@echo "Creating ext2 disk image..."
	dd if=/dev/zero of=$(EXT2_IMG) bs=1M count=$(EXT2_SIZE) status=none
	mkfs.ext2 -F -b 4096 -O ^64bit,^metadata_csum,^flex_bg $(EXT2_IMG)
	@echo "ext2 disk image created."

# --- QEMU ------------------------------------------------------------------

run: eos.img $(EXT2_IMG)
	qemu-system-x86_64 \
		-drive file=eos.img,format=raw,if=floppy \
		-drive file=$(EXT2_IMG),format=raw,if=none,id=ext2disk \
		-device ahci,id=ahci \
		-device ide-hd,drive=ext2disk,bus=ahci.0 \
		-boot a

debug: eos.img kernel.elf
	qemu-system-x86_64 -s -S \
		-drive file=eos.img,format=raw,if=none,id=boot \
		-device ahci,id=ahci \
		-device ide-hd,drive=boot,bus=ahci.0 \
		-boot c -m 512 &
	$(GDB) -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

clean:
	rm -f eos.img os-image.bin kernel.bin kernel.elf ext2_disk.img
	rm -f boot/*.bin boot/*.o kernel/*.o drivers/*.o cpu/*.o libc/*.o \
	      syscall/*.o multitasking/*.o