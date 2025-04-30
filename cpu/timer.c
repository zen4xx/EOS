#include "timer.h"

u32 tick = 0;

static void timer_callback(registers_t reg) {
    ++tick;
}

void init_timer(u32 freq){
    register_interrupt_handler(IRQ0, timer_callback);

    //1193180hz is hardware clock
    u32 div = 1193180 / freq;
    u8 low = (u8)(div & 0xFF);
    u8 high = (u8)((div >> 8) & 0xFF);

    port_byte_out(0x43, 0x36); //cmd port
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}