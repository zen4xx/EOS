#ifndef KERNEL_H
#define KERNEL_H

#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../libc/libc.h"
#include "../drivers/keyboard.h"

void exec(char* cmd);

#endif