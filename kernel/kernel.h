#ifndef KERNEL_H
#define KERNEL_H

#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"

#define VGA_BLACK 0
#define VGA_BLUE 1
#define VGA_GREEN 2
#define VGA_CYAN 3
#define VGA_RED 4
#define VGA_MAGENTA 5
#define VGA_ORANGE 6
#define VGA_LIGHTGREY 7
#define VGA_GREY 8
#define VGA_VIOLET 9
#define VGA_LIGHTGREEN a
#define VGA_LIGHTCYAN b
#define VGA_LIGHTRED c
#define VGA_LIGHTMAGENTA d
#define VGA_YELLOW e
#define VGA_WHITE f

#define INPUT_BUF_SIZE 256

#define COMBINE_impl(a, b) 0x##b##a
//connects fg and bg
#define COMBINE(fg, bg) COMBINE_impl(fg,bg) 

void exec(char* cmd);

extern char _current_char;

#endif
