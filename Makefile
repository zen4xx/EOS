# ===========================================================================
# EOS build
#
#   make            -> eos.img   (a full 1.44 MB floppy image, dd-able)
#   make run        -> boot it in QEMU as a floppy
#   make clean
#
# Layout of eos.img:
#   LBA 0        stage 1 boot sector   (512 B, has a BPB)
#   LBA 1..4     stage 2 loader        (2 KiB)
#   LBA 5..      kernel.bin            (loaded to 0x1000)
# ===========================================================================

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c syscall/*.c multitasking/*.c)
HEADERS   = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h syscall/*.h multitasking/*.h)
OBJ       = $(C_SOURCES:.c=.o) cpu/interrupts.o

# Override on the command line if you build with a host toolchain, e.g.
#   make CC="gcc" LD="ld" OBJCOPY="objcopy"
CC      = i686-elf-gcc
LD      = i686-elf-ld
OBJCOPY = i686-elf-objcopy
GDB     = i686-elf-gdb

CFLAGS  = -m32 -g -Wall -ffreestanding -nostdlib -fno-builtin \
          -fno-pie -fno-pic -fno-stack-protector \
          -fno-asynchronous-unwind-tables
LDFLAGS = -m elf_i386 -T boot/link.ld -nostdlib

FLOPPY_BYTES = 1474560

.PHONY: all run clean
all: eos.img

eos.img: boot/stage1.bin boot/stage2.bin kernel.bin
	cat boot/stage1.bin boot/stage2.bin kernel.bin > $@
	@truncate -s $(FLOPPY_BYTES) $@
	@echo "eos.img ready ($(FLOPPY_BYTES) bytes)"

boot/stage1.bin: boot/stage1.asm
	nasm $< -f bin -o $@

# stage 2 has to be told how big the kernel is, so it is built last.
boot/stage2.bin: boot/stage2.asm kernel.bin
	@SZ=$$(stat -c%s kernel.bin); SECT=$$(( ($$SZ + 511) / 512 )); \
	 echo "kernel.bin = $$SZ bytes -> $$SECT sectors"; \
	 if [ $$SECT -gt 2000 ]; then echo "kernel too big for a floppy"; exit 1; fi; \
	 nasm $< -f bin -D KERNEL_SECTORS=$$SECT -o $@

kernel.elf: boot/kernel_entry.o $(OBJ) boot/link.ld
	$(LD) $(LDFLAGS) -o $@ boot/kernel_entry.o $(OBJ)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm $< -f elf32 -o $@

run: eos.img
	qemu-system-i386 -drive file=eos.img,format=raw,if=floppy -boot a

debug: eos.img kernel.elf
	qemu-system-i386 -s -S -drive file=eos.img,format=raw,if=floppy -boot a &
	$(GDB) -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

clean:
	rm -f eos.img os-image.bin kernel.bin kernel.elf
	rm -f boot/*.bin boot/*.o kernel/*.o drivers/*.o cpu/*.o libc/*.o \
	      syscall/*.o multitasking/*.o
