#include "isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../drivers/ports.h"
#include "timer.h"
#include "../drivers/keyboard.h"

static isr_t interrupt_handlers[256];

void isr_install(void) {
    /* CPU exceptions (ISR 0-31) */
    set_idt_gate(0,  (u64)isr0);
    set_idt_gate(1,  (u64)isr1);
    set_idt_gate(2,  (u64)isr2);
    set_idt_gate(3,  (u64)isr3);
    set_idt_gate(4,  (u64)isr4);
    set_idt_gate(5,  (u64)isr5);
    set_idt_gate(6,  (u64)isr6);
    set_idt_gate(7,  (u64)isr7);
    set_idt_gate(8,  (u64)isr8);
    set_idt_gate(9,  (u64)isr9);
    set_idt_gate(10, (u64)isr10);
    set_idt_gate(11, (u64)isr11);
    set_idt_gate(12, (u64)isr12);
    set_idt_gate(13, (u64)isr13);
    set_idt_gate(14, (u64)isr14);
    set_idt_gate(15, (u64)isr15);
    set_idt_gate(16, (u64)isr16);
    set_idt_gate(17, (u64)isr17);
    set_idt_gate(18, (u64)isr18);
    set_idt_gate(19, (u64)isr19);
    set_idt_gate(20, (u64)isr20);
    set_idt_gate(21, (u64)isr21);
    set_idt_gate(22, (u64)isr22);
    set_idt_gate(23, (u64)isr23);
    set_idt_gate(24, (u64)isr24);
    set_idt_gate(25, (u64)isr25);
    set_idt_gate(26, (u64)isr26);
    set_idt_gate(27, (u64)isr27);
    set_idt_gate(28, (u64)isr28);
    set_idt_gate(29, (u64)isr29);
    set_idt_gate(0x30, (u64)sched_isr);  /* Scheduler interrupt */
    set_idt_gate(31, (u64)isr31);
    set_idt_gate(0x80, (u64)isr80);      /* System call interrupt */

    port_byte_out(0x20, 0x11);  /* ICW1: init + ICW4 needed */
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20);  /* ICW2: master PIC offset to 0x20 (32) */
    port_byte_out(0xA1, 0x28);  /* ICW2: slave PIC offset to 0x28 (40) */
    port_byte_out(0x21, 0x04);  /* ICW3: master has slave on IRQ2 */
    port_byte_out(0xA1, 0x02);  /* ICW3: slave cascade identity */
    port_byte_out(0x21, 0x01);  /* ICW4: 8086 mode */
    port_byte_out(0xA1, 0x01);

    port_byte_out(0x21, 0xFB);  /* Unmask IRQ2 (cascade) */
    port_byte_out(0xA1, 0xFF);  /* Mask all slave IRQs */

    /* Hardware interrupts (IRQ 0-15 -> ISR 32-47) */
    set_idt_gate(32, (u64)irq0);
    set_idt_gate(33, (u64)irq1);
    set_idt_gate(34, (u64)irq2);
    set_idt_gate(35, (u64)irq3);
    set_idt_gate(36, (u64)irq4);
    set_idt_gate(37, (u64)irq5);
    set_idt_gate(38, (u64)irq6);
    set_idt_gate(39, (u64)irq7);
    set_idt_gate(40, (u64)irq8);
    set_idt_gate(41, (u64)irq9);
    set_idt_gate(42, (u64)irq10);
    set_idt_gate(43, (u64)irq11);
    set_idt_gate(44, (u64)irq12);
    set_idt_gate(45, (u64)irq13);
    set_idt_gate(46, (u64)irq14);
    set_idt_gate(47, (u64)irq15);

    /* Load the IDT into the CPU */
    set_idt();
}

/* Exception messages for ISR 0-31 */
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved"
};


void irq_set_mask(u8 irq, u8 masked) {
    u16 port = (irq < 8) ? 0x21 : 0xA1;
    u8  bit  = (irq < 8) ? irq : (u8)(irq - 8);
    u8  cur  = port_byte_in(port);

    if (masked) {
        cur |= (u8)(1 << bit);
    } else {
        cur &= (u8)~(1 << bit);
    }

    port_byte_out(port, cur);
}

void isr_handler(registers_t* reg) {
    if (reg->int_no <= 31) {
        krnl_print("Exception: ");
        krnl_print(exception_messages[reg->int_no]);
        krnl_print(" (int ");
        
        char s[16];
        itoa((int)reg->int_no, s);
        krnl_print(s);
        
        if (reg->err_code != 0) {
            krnl_print(", error code ");
            itoa((int)reg->err_code, s);
            krnl_print(s);
        }
        
        krnl_print(")\n");
        krnl_print("RIP: 0x");
        itoa((int)(reg->rip >> 32), s);
        krnl_print(s);
        itoa((int)(reg->rip & 0xFFFFFFFF), s);
        krnl_print(s);
        krnl_print("\n");
    }
    
    /* Halt on fatal exceptions */
    if (reg->int_no == 8 || reg->int_no == 13 || reg->int_no == 14) {
        krnl_print("Fatal exception, halting.\n");
        asm volatile("cli; hlt");
    }
}

void register_interrupt_handler(u8 n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void irq_handler(registers_t* reg) {
    u32 irq = (u32)(reg->int_no - 32);

    if (irq == 7 || irq == 15) {
        u16 pic = (irq == 7) ? 0x20 : 0xA0;
        port_byte_out(pic, 0x0B);
        
        if (!(port_byte_in(pic) & 0x80)) {
            if (irq == 15) {
                port_byte_out(0x20, 0x20);
            }
            return;
        }
    }

    if (reg->int_no >= 40) {
        port_byte_out(0xA0, 0x20);
    }
    port_byte_out(0x20, 0x20);

    if (interrupt_handlers[reg->int_no] != NULL) {
        isr_t handler = interrupt_handlers[reg->int_no];
        handler(reg);
    }
}

void irq_install(void) {
    init_timer(500);     /* IRQ0: timer at 500 Hz */
    init_keyboard();     /* IRQ1: keyboard */
    
    asm volatile("sti");
}