#include "kernel.h"
#include "alloc.h"
#include "pmm.h"
#include "../multitasking/mt.h"
#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/gdt.h"
#include "../libc/string.h"
#include "../libc/stdint.h"
#include "../drivers/ahci.h"
#include "../fs/ext2.h"
#include "../fs/vfs.h"
#include "progs.h"

volatile char _current_char = '\0';
volatile char _is_current_task_foreground = 0;

ext2_fs_t fs;

void kernel_main() {

    gdt_init();
    tss_init();

	clear();

    isr_install();
    irq_install();

    init_allocator();

    pmm_init(0x800000, 0x800000);

    int res = ahci_init();
    if (res != 0)
    {
        krnl_print("failed to init ahci with err code: ");
        char str[10];
        itoa(res, str);
        krnl_print(str);
        krnl_print("\n halting \n");
        __asm__ volatile("hlt");
    }

    vfs_init();

    res = vfs_mount_ext2_root();
    if (res != 0)
    {
        krnl_print("failed to mount ext2 with err code: ");
        char str[10];
        itoa(res, str);
        krnl_print(str);
        krnl_print("\n halting \n");
        __asm__ volatile("hlt");
    }

    krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));

    task_create_ex(idle_task, 0, 0, 1024, 0, 0);
    task_create(shell, 0, 0, 0, 0);
    start_first_task();
}
