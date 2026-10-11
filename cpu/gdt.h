#ifndef GDT_H
#define GDT_H

#include "../libc/stdint.h"

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define KERNEL_SS 0x10

#define USER_CS   0x1B
#define USER_DS   0x23
#define USER_SS   0x23

#define TSS_SEG   0x28

void tss_init(void);
void gdt_init(void);
void tss_set_kernel_stack(uint64_t stack_top);

#endif 