#include "gdt.h"
#include "../libc/memory.h"

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t gran;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) gdt_tss_entry_t;

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

gdt_entry_t gdt_entries[7]; // null, kernel code, kernel data, user code, user data, tss, tss
gdt_ptr_t gdt_ptr;
__attribute__((aligned(16))) tss_t kernel_tss;

extern void gdt_flush(uint64_t gdt_ptr_addr);

void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    
    gdt_entries[num].gran = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt_entries[num].access = access;
}

void tss_set_entry(int num, uint64_t base, uint32_t limit) {
    gdt_entries[num].limit_low    = limit & 0xFFFF;
    gdt_entries[num].base_low     = base & 0xFFFF;
    gdt_entries[num].base_middle  = (base >> 16) & 0xFF;
    gdt_entries[num].access       = 0x89;
    gdt_entries[num].gran         = (limit >> 16) & 0x0F;
    gdt_entries[num].base_high    = (base >> 24) & 0xFF;
    
    uint32_t* upper = (uint32_t*)&gdt_entries[num + 1];
    upper[0] = base >> 32;
    upper[1] = 0;
}

void gdt_init(void)
{
    gdt_set_entry(0, 0, 0, 0, 0);             // null
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0); // kernel code
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0); // kernel data
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0); // user code
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0); // user data
    gdt_set_entry(5, 0, 0, 0, 0);             //tss 
    gdt_set_entry(6, 0, 0, 0, 0);             //tss 

    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = (uint64_t)&gdt_entries;

    gdt_flush((uint64_t)&gdt_ptr);
}

void tss_init(void)
{
    memset(&kernel_tss, 0, sizeof(tss_t));
    kernel_tss.iomap_base = sizeof(tss_t);
    kernel_tss.rsp0 = 0x00090000; // kernel_entry.asm
    tss_set_entry(5, (uint64_t)&kernel_tss, sizeof(tss_t) - 1);

    uint16_t tss_selector = 0x28;

    __asm__ volatile("ltr %0" : : "r"(tss_selector));
}

void tss_set_kernel_stack(uint64_t stack_top) {
    kernel_tss.rsp0 = stack_top;
}