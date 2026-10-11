#include "idt.h"

idt_gate_t idt[IDT_ENTRIES] __attribute__((aligned(16)));
idt_register_t idt_reg;

void set_idt_gate_ex(int n, u64 handler, u8 dpl) {
    idt[n].offset_low  = low_16(handler);
    idt[n].sel         = KERNEL_CS;
    idt[n].ist         = 0;          /* no IST used */
    idt[n].type_attr   = 0x80 | ((dpl & 0x03) << 5) | 0x0E;       /* P=1 (0x80), DPL=dpl, type=0xE (64-bit interrupt gate) */
    idt[n].offset_mid  = mid_16(handler);
    idt[n].offset_high = high_32(handler);
    idt[n].reserved    = 0;
}

void set_idt_gate(int n, u64 handler) {
    set_idt_gate_ex(n, handler, 0);
}

void set_idt(void) {
    idt_reg.limit = (u16)(IDT_ENTRIES * sizeof(idt_gate_t) - 1);
    idt_reg.base  = (u64)&idt;

    asm volatile("cli");
    asm volatile("lidt %0" : : "m"(idt_reg));
}