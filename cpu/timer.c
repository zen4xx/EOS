#include "timer.h"

static inline void yield(void)
{
    asm volatile("int $0x30" ::: "memory");
}

volatile u64 tick = 0;

static void timer_callback(registers_t* reg) {
    (void)reg;
    
    ++tick;
    
    yield(); 
}

void init_timer(u32 freq) {
    register_interrupt_handler(IRQ0, timer_callback);

    /* 1193180 Hz is the base PIT hardware clock frequency */
    if (freq == 0) freq = 1;
    
    u32 div = 1193180 / freq;
    u8 low  = (u8)(div & 0xFF);
    u8 high = (u8)((div >> 8) & 0xFF);

    /* 
     * Command port 0x43:
     * 0x36 = 00110110b
     *   Channel 0 (00)
     *   Access mode: lobyte/hibyte (11)
     *   Operating mode: rate generator (011)
     *   BCD/Binary mode: 16-bit binary (0)
     */
    port_byte_out(0x43, 0x36); 
    
    /* Data port 0x40: send divisor */
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);

    /* Unmask IRQ0 (timer) in the PIC */
    irq_set_mask(0, 0);
}

u64 get_ticks(void) {
    return tick;
}