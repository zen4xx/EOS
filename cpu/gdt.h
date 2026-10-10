#ifndef GDT_H
#define GDT_H

#include "../libc/stdint.h"

void tss_init(void);
void gdt_init(void);
void tss_set_kernel_stack(uint64_t stack_top);

#endif 