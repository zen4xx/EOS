#ifndef ISR_H
#define ISR_H

#include "types.h"
#include "idt.h"

typedef struct {
    /* Pushed by our assembly stub (pushaq) */
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    /* Pushed by stub: data segment (for compatibility) */
    u64 ds;
    
    /* Interrupt number and error code (pushed by stub) */
    u64 int_no, err_code;

    /* Pushed automatically by CPU */
    u64 rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

typedef void (*isr_t)(registers_t*);

/* ISR declarations (implemented in assembly) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr31(void);
extern void isr80(void);

/* IRQ declarations (implemented in assembly) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

extern void sched_isr(void);

#define IRQ0  32
#define IRQ1  33
#define IRQ2  34
#define IRQ3  35
#define IRQ4  36
#define IRQ5  37
#define IRQ6  38
#define IRQ7  39
#define IRQ8  40
#define IRQ9  41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

void isr_install(void);
void irq_install(void);
void isr_handler(registers_t* reg);
void irq_handler(registers_t* reg);
void irq_set_mask(u8 irq, u8 masked);
void register_interrupt_handler(u8 n, isr_t handler);

#endif