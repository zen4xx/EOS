#ifndef SCREEN_H
#define SCREEN_H

#include "../libc/stdint.h"
#include "../libc/stddef.h"

#define VIDEO_ADDRESS ((volatile uint8_t*)0xb8000)
#define MAX_ROWS 25
#define MAX_COLS 80

#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5 

// kernel api
void clear(void);
void krnl_print_at(const char *msg, int col, int row, uint8_t color);
void krnl_print(const char *msg);
size_t print_char(char c, int col, int row, uint8_t attr); 

#endif