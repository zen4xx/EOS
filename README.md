# EOS

**E**chobreak **O**perating **S**ystem — a 32-bit x86 hobby kernel with a
two-stage BIOS bootloader, a VGA text console, PS/2 keyboard input, an `int
0x80` syscall layer, a heap allocator and a small interactive shell.

Boots on QEMU and on real hardware, from a floppy or a USB stick.

```
W3lC0M3 T0 EOS
>help
Type shutdown to shutdown
Type reboot to reboot
...
>calc 7 * 6
42
>
```

---

## Contents

- [Quick start](#quick-start)
- [Toolchain](#toolchain)
- [Building](#building)
- [Running under QEMU](#running-under-qemu)
- [Tests](#tests)
- [Debugging with GDB](#debugging-with-gdb)
- [Real hardware](#real-hardware)
- [How it boots](#how-it-boots)
- [Memory map](#memory-map)
- [Repository layout](#repository-layout)
- [Shell commands](#shell-commands)
- [Syscall ABI](#syscall-abi)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)

---

## Quick start

```bash
git clone https://github.com/zen4xx/EOS.git
cd EOS
make            # -> eos.img
make run        # boot it in QEMU
make test       # headless boot tests across all three emulation paths
```

---

## Toolchain

You need **nasm**, **make**, **qemu-system-i386**, and an **i686-elf** cross
compiler. The cross compiler matters: a host `gcc` targets your Linux ABI,
defaults to PIE, and will happily link things a freestanding kernel cannot use.

### Arch

```bash
sudo pacman -S nasm make qemu-system-x86
yay -S i686-elf-gcc i686-elf-binutils i686-elf-gdb   # AUR
```

### macOS

```bash
brew install nasm qemu i686-elf-gcc i686-elf-binutils i686-elf-gdb
```

### Debian / Ubuntu

No packaged cross compiler, so build one. Takes 15–30 minutes, once.

```bash
sudo apt install build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev \
                 texinfo nasm make qemu-system-x86 wget

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p ~/src && cd ~/src
# check ftp.gnu.org for current versions
wget https://ftp.gnu.org/gnu/binutils/binutils-2.43.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.gz
tar xf binutils-2.43.tar.gz && tar xf gcc-14.2.0.tar.gz

mkdir build-binutils && cd build-binutils
../binutils-2.43/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror
make -j$(nproc) && make install && cd ..

cd gcc-14.2.0 && ./contrib/download_prerequisites && cd ..
mkdir build-gcc && cd build-gcc
../gcc-14.2.0/configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c --without-headers
make -j$(nproc) all-gcc all-target-libgcc
make install-gcc install-target-libgcc
```

Add `export PATH="$HOME/opt/cross/bin:$PATH"` to your shell rc file.

### Building without a cross compiler

Fine for a quick check, not for anything you intend to push:

```bash
make CC="gcc -m32 -fno-pie -fno-pic" LD=ld OBJCOPY=objcopy
```

On 64-bit Debian/Ubuntu you also need `gcc-multilib`.

---

## Building

```bash
make            # eos.img — a 1474560-byte floppy image, dd-able as-is
make clean
```

Override the toolchain prefix if yours isn't on `PATH` as `i686-elf-*`:

```bash
make CC=i386-elf-gcc LD=i386-elf-ld OBJCOPY=i386-elf-objcopy
```

The build measures `kernel.bin` and passes the real sector count into stage 2,
so the loader can never read fewer sectors than the kernel needs. It also
fails if the kernel would grow past 26624 bytes, because it loads at `0x1000`
and would collide with stage 2's real-mode stack at `0x7C00`.

---

## Running under QEMU

Firmware hands a removable device to the boot sector in one of several ways,
and each exercises a different path in the loader. All three targets use the
same `eos.img`.

| Target | Emulates | `DL` | Read path |
|---|---|---|---|
| `make run` | floppy — what a USB floppy adapter looks like | `0x00` | CHS, 18 spt / 2 heads |
| `make run-usb` | USB mass storage | BIOS choice | whatever SeaBIOS reports |
| `make run-hdd` | hard disk — what a USB stick usually looks like | `0x80` | LBA via `INT 13h AH=42h` |

Raw commands, if you'd rather not go through make:

```bash
# floppy
qemu-system-i386 -drive file=eos.img,format=raw,if=floppy -boot a

# USB mass storage
qemu-system-i386 -drive if=none,id=stick,format=raw,file=eos.img \
                 -usb -device usb-storage,drive=stick -boot c

# hard disk
qemu-system-i386 -drive file=eos.img,format=raw,if=ide,index=0 -boot c
```

Useful extras:

```bash
-d guest_errors,int   # log interrupts and guest faults
-no-reboot            # halt instead of rebooting on triple fault
-no-shutdown          # keep the window after a shutdown
-display none         # headless; drive it over -monitor stdio
```

Ctrl-Alt-G releases the mouse, Ctrl-Alt-2 gets you the QEMU monitor.

---

## Tests

```bash
make test               # everything
./tests/smoke.sh usb    # one case: floppy | usb | hdd | shell
```

`tests/smoke.sh` boots the image headless, reads the VGA text buffer straight
out of guest memory over the QEMU monitor, and asserts on what's on screen. No
display required, so it runs over ssh and in CI.

It covers:

- boots to the shell under floppy, USB-storage and hard-disk emulation
- `help` runs — keyboard IRQ, scancode map, shell parser
- `calc 7 * 6` gives `42` — malloc, the vector, `atoi`/`itoa`
- `meminfo` responds — heap above 1 MB, so A20 is genuinely on

```
floppy  (USB-FDD emulation: DL=0x00, CHS reads, 18spt/2heads)
  PASS  boots to shell
usb     (USB mass storage, geometry decided by the BIOS)
  PASS  boots to shell
hdd     (USB-HDD emulation: DL=0x80, LBA reads via INT 13h AH=42h)
  PASS  boots to shell
shell   (keyboard IRQ, parser, heap above 1 MB)
  PASS  help runs
  PASS  calc 7 * 6 = 42
  PASS  allocator alive

6 passed, 0 failed
```

Bump the boot wait if your machine is slow: `BOOT_WAIT=10 make test`.

---

## Debugging with GDB

```bash
make debug
```

Starts QEMU stopped at the reset vector with a gdbstub on `:1234` and attaches
`i686-elf-gdb` with `kernel.elf` loaded, so you get real symbols.

```gdb
(gdb) b kernel_main
(gdb) c
(gdb) info registers
(gdb) x/16xb 0xb8000
```

To step through the bootloader instead, stay in 16-bit mode:

```gdb
(gdb) set architecture i8086
(gdb) b *0x7c00
(gdb) c
(gdb) x/20i $cs*16+$eip
```

`kernel.elf` is regenerated by every build, so re-run `symbol-file kernel.elf`
if you rebuilt while GDB was attached.

---

## Real hardware

`eos.img` boots as-is from either media type. Stage 1 carries both a BPB (for
USB-FDD emulation) and an MBR partition table (for USB-HDD emulation), and
stage 2 probes the geometry rather than assuming it, so one image covers both.

### Writing it

**Floppy, or a real floppy in a USB adapter:**

```bash
lsblk                                  # find the device
sudo dd if=eos.img of=/dev/sdX bs=512 conv=fsync status=progress
sudo eject /dev/sdX
```

**USB flash drive:**

```bash
lsblk
sudo dd if=eos.img of=/dev/sdX bs=1M conv=fsync status=progress
sudo sync
```

Write to the **device** (`/dev/sdX`), never a partition (`/dev/sdX1`). Run
`lsblk` and read it before you press enter — `dd` onto the wrong device takes
your system disk with it. On Windows, use Rufus in DD-image mode.

Only the first 1.44 MB is written. Whatever else was on the stick stays there
and is ignored.

### BIOS settings

Legacy boot has to be alive, which on a modern board means:

- **Secure Boot: Disabled** — CSM won't enable otherwise
- **Fast Boot: Disabled**
- **CSM / Launch CSM: Enabled**
- **Boot Device Control: Legacy only** (or UEFI *and* Legacy)
- **Storage OpROM policy: Legacy only**
- **Legacy USB Support / XHCI Hand-off: Enabled** — this is what provides
  `INT 13h` for USB media and 8042 emulation for a USB keyboard

In the boot menu, prefer a `USB-FDD` or plain-device entry over `USB-ZIP`. ZIP
emulation expects the boot code in partition 4 at a fixed offset and won't work
here. Some firmware also only lists USB devices plugged in at power-on, not
ones added during a warm reboot.

Two hardware caveats worth knowing:

- **A GPU with no legacy VBIOS gives a black screen under CSM.** NVIDIA has
  shipped UEFI-only VBIOSes for several generations. If your CPU has no
  integrated graphics there's no fallback, and an older PCIe card with a legacy
  VBIOS is the fix.
- **Some 600-series and newer boards ship with CSM removed entirely.** If
  there's no CSM option in your firmware, legacy boot is impossible and the
  only route is a UEFI stub loader instead of a boot sector.

### What you should see

The loader is deliberately chatty, so a failure tells you where it stopped:

```
EOS s1                          <- stage 1 ran, segments are sane
EOS s2: loading kernel ....     <- one dot per successful read
entering protected mode
W3lC0M3 T0 EOS                  <- kernel is alive
>
```

---

## How it boots

```
BIOS ──► stage 1  (LBA 0, 512 B, loaded to 0x7C00)
         · zero DS/ES/SS, set SP, far jump to normalise CS:IP
         · sanity check the BIOS drive number in DL
         · load stage 2 from C0/H0/S2..5, with retries
              │
              ▼
         stage 2  (LBA 1..4, 2 KiB, at 0x7E00)
         · INT 10h AX=0003h — force 80x25 text, known console state
         · INT 13h AH=41h  — are LBA reads available?
         · INT 13h AH=08h  — real drive geometry
         · load kernel.bin to 0x1000: LBA, or track-by-track CHS,
           5 retries with a controller reset between attempts,
           never crossing a track or a 64K DMA boundary
         · enable A20 (BIOS 15h/2401 → port 0x92 → 8042, all with timeouts)
         · load GDT, set CR0.PE, far jump to 32-bit
              │
              ▼
         kernel_entry  (0x1000)
         · set up the stack at 0x90000
         · zero .bss — it is not present in kernel.bin, and real firmware
           does not hand you zeroed RAM
         · call kernel_main()
              │
              ▼
         kernel_main
         · clear() → isr_install() → irq_install() → init_allocator()
         · shell loop, driven by the keyboard IRQ
```

Image layout:

```
LBA 0        stage1.bin   512 B    boot sector, BPB + MBR partition table
LBA 1..4     stage2.bin   2 KiB    loader
LBA 5..      kernel.bin            loaded to 0x1000
             padding               up to 1474560 bytes
```

---

## Memory map

| Range | Contents |
|---|---|
| `0x00000500`–`0x00007C00` | free; real-mode stack grows down from `0x7C00` |
| `0x00001000`–`0x00006800` | kernel `.text` / `.rodata` / `.data` / `.bss` |
| `0x00007C00`–`0x00007E00` | stage 1 |
| `0x00007E00`–`0x00008600` | stage 2 |
| `0x00090000` | protected-mode stack top (grows down) |
| `0x000B8000` | VGA text framebuffer, 80×25×2 bytes |
| `0x00100000`–`0x00800000` | kernel heap |

The kernel loads at `0x1000` and the real-mode stack lives at `0x7C00`, which
caps `kernel.bin` at 26624 bytes. The build enforces this. To raise the
ceiling, load above 1 MB and copy down after entering protected mode, or
relocate the real-mode stack in `boot/stage2.asm`.

---

## Repository layout

```
boot/
  stage1.asm        boot sector: segments, BPB, MBR table, loads stage 2
  stage2.asm        geometry probe, kernel load, A20, GDT, protected mode
  kernel_entry.asm  stack, .bss zeroing, calls kernel_main
  link.ld           kernel layout, exports __bss_start / __bss_end
kernel/
  kernel.c          kernel_main and the shell command table
  kernel_api.c      line editing on top of the keyboard driver
  alloc.c           free-list heap allocator
cpu/
  interrupts.asm    ISR/IRQ stubs
  idt.c  isr.c      IDT, PIC remap, handler dispatch
  timer.c           PIT on IRQ0
drivers/
  screen.c          VGA text console, cursor, scrolling
  keyboard.c        PS/2 scancode set 1
  ports.c           in/out wrappers
libc/
  string  stdlib  stdio  memory  vect  jmp  error  power
syscall/
  syscall.c         int 0x80 dispatch
multitasking/
  mt.c              cooperative tasks via setjmp-style buffers
tests/
  smoke.sh          headless QEMU boot tests
```

---

## Shell commands

| Command | Effect |
|---|---|
| `help` | list commands |
| `clear` | clear the screen |
| `echo <words...>` | print the arguments |
| `calc <a> <op> <b>` | integer `+ - * /` |
| `meminfo` | total allocated bytes |
| `honorboard` | credits |
| `reboot` | reset via the 8042 |
| `shutdown` | ACPI poweroff, then halt |
| `animegirl`, `blackjack` | ask and find out |

---

## Syscall ABI

`int 0x80`, number in `EAX`, arguments in `EBX ECX EDX ESI EDI EBP`, return
value in `EAX`.

| `EAX` | Name | Arguments | Returns |
|---|---|---|---|
| 1 | `PRINT_STRING` | `EBX` = `char*` | 0 |
| 2 | `GET_TIME` | — | ticks |
| 10 | `MALLOC` | `EBX` = size | pointer |
| 11 | `REALLOC` | `EBX` = ptr, `ECX` = size | pointer |
| 12 | `FREE` | `EBX` = ptr | 0 |
| 13 | `MALLOC_STATS` | — | bytes allocated |
| 30 | `GETCHAR` | — | character (blocks) |
| 31 | `CLEAR` | — | 0 |
| 100 | `POWER_OFF` | — | does not return |
| 101 | `REBOOT` | — | does not return |

Unknown numbers return `(u32)-1`.

---

## Troubleshooting

| Symptom | Cause |
|---|---|
| Nothing on screen at all | Firmware never executed the sector. CSM/legacy boot problem, or a GPU with no legacy VBIOS. |
| `EOS s1` then `S1: disk error` | The BIOS can't read LBA 1–4. Wrong emulation type, or bad media. |
| Dots stop, `S2: disk read failed` | Media or geometry. Try another floppy/stick first. |
| `S2: A20 could not be enabled` | Rare. Open an issue with the board model. |
| Reaches `entering protected mode`, then nothing | The loader did its job; the bug is in the kernel. |
| Boots in QEMU, hangs on hardware | Almost always something relying on QEMU's zeroed RAM, a missing retry, or a multi-track `INT 13h` read. |
| Keyboard dead on hardware, fine in QEMU | 8042 output buffer not drained, or Legacy USB Support disabled in the BIOS. |
| `make` fails with `_GLOBAL_OFFSET_TABLE_` undefined | You're building with a host `gcc` that defaults to PIE. Use the cross compiler, or add `-fno-pie -fno-pic`. |

---

## Contributing

1. Branch off `main`: `git checkout -b fix/thing`
2. `make && make test` — all six cases must pass
3. If you touched the boot path, test on real hardware too. QEMU will not catch
   geometry, retry or uninitialised-memory bugs
4. Keep commits focused. Bootloader changes and driver changes are easier to
   review and revert separately
5. Build artefacts are gitignored — don't commit `eos.img` or `kernel.elf`

Roadmap lives in [`roadmap`](roadmap).
