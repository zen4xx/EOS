#include "idt.h"

idt_gate_t idt[IDT_ENTRIES] __attribute__((aligned(16)));
idt_register_t idt_reg;

void set_idt_gate(int n, u64 handler) {
    idt[n].offset_low  = low_16(handler);
    idt[n].sel         = KERNEL_CS;
    idt[n].ist         = 0;          /* no IST used */
    idt[n].type_attr   = 0x8E;       /* P=1, DPL=0, type=0xE (64-bit interrupt gate) */
    idt[n].offset_mid  = mid_16(handler);
    idt[n].offset_high = high_32(handler);
    idt[n].reserved    = 0;
}

void set_idt(void) {
    idt_reg.limit = (u16)(IDT_ENTRIES * sizeof(idt_gate_t) - 1);
    idt_reg.base  = (u64)&idt;

    asm volatile("cli");
    asm volatile("lidt %0" : : "m"(idt_reg));
}