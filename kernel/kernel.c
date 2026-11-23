#include "kernel.h"
#include "alloc.h"
#include "pmm.h"
#include "progs.h"
#include "paging.h"
#include "e820.h"
#include "../multitasking/mt.h"
#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/gdt.h"
#include "../libc/string.h"
#include "../libc/stdint.h"
#include "../drivers/ahci.h"
#include "../fs/ext2.h"
#include "../fs/vfs.h"
#include "../fs/devfs.h"

volatile char _current_char = '\0';
volatile char _is_current_task_foreground = 0;

void kernel_main() {

    gdt_init();
    tss_init();

	clear();

    isr_install();
    irq_install();

    const e820_map_t* map = e820_get();

    uint64_t pool_base = 0x800000;
    uint64_t pool_end  = pool_base + 8 * 1024 * 1024;   // fallback

    if (map) {
        pool_end = pool_base;
        for (uint32_t i = 0; i < map->count; i++)
            if (map->entries[i].type == E820_USABLE) {
                uint64_t e = map->entries[i].base + map->entries[i].length;
                if (e > pool_end) pool_end = e;
            }
    }

    pmm_init(pool_base, pool_end - pool_base);

    if (map) {
        for (uint32_t i = 0; i < map->count; i++)
            if (map->entries[i].type == E820_USABLE)
                pmm_init_region(map->entries[i].base, map->entries[i].length);
        for (uint32_t i = 0; i < map->count; i++)
            if (map->entries[i].type != E820_USABLE)
                pmm_reserve(map->entries[i].base, map->entries[i].length);
    } else {
        krnl_print("warning: no e820 map, assuming 16 MiB of RAM\n");
        pmm_init_region(0x800000, 8 * 1024 * 1024);
    }

    paging_init();
    paging_identity_extend(pool_end);
    init_allocator();

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

    res = devfs_init();
    if (res != 0)
        krnl_print("warning: failed to init devfs");

    krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));

    task_create_ex(idle_task, 0, 0, 1024, 0, 0, NULL, NULL, NULL);
    task_create(shell, 0, 0, 0, 0);
    start_first_task();
}
