#ifndef IDT_H
#define IDT_H

#include "types.h"

#define KERNEL_CS 0x18

typedef struct {
    u16 offset_low;
    u16 sel;
    u8  ist;
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
} __attribute__((packed)) idt_gate_t;

typedef struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256

extern idt_gate_t idt[IDT_ENTRIES];
extern idt_register_t idt_reg;

void set_idt_gate(int n, u64 handler);
void set_idt(void);

#endif