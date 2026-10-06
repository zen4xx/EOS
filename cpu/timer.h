#ifndef TIMER_H
#define TIMER_H

#include "../libc/string.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "isr.h"

void init_timer(u32 freq);

u64 get_ticks(void);

#endif