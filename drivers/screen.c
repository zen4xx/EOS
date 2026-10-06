#include "screen.h"
#include "ports.h"
#include "../libc/memory.h" 

static size_t get_cursor_offset(void);
static void set_cursor_offset(size_t offset);
static size_t get_offset(int col, int row);
static int get_offset_row(size_t offset);
static int get_offset_col(size_t offset);

void krnl_print_at(const char *msg, int col, int row, uint8_t color) {
    size_t offset;
    if (col >= 0 && row >= 0) {
        offset = get_offset(col, row);
    } else {
        offset = get_cursor_offset();
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }

    int i = 0;
    while (msg[i] != 0) {
        offset = print_char(msg[i++], col, row, color);
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }
}

void krnl_print(const char *msg) {
    krnl_print_at(msg, -1, -1, 0x0f);
}

size_t print_char(char c, int col, int row, uint8_t attr) {
    volatile uint8_t *vidmem = VIDEO_ADDRESS;
    if (!attr) attr = 0x0f;

    if (col >= MAX_COLS || row >= MAX_ROWS) {
        size_t err_offset = get_offset(MAX_COLS - 1, MAX_ROWS - 1);
        vidmem[err_offset] = 'E';
        vidmem[err_offset + 1] = 0x04;
        return get_offset(col, row);
    }

    size_t offset;
    if (col >= 0 && row >= 0) {
        offset = get_offset(col, row);
    } else {
        offset = get_cursor_offset();
    }

    if (c == '\n') {
        row = get_offset_row(offset);
        offset = get_offset(0, row + 1);
    } else if (c == '\b') {
        if (offset >= 2) {
            vidmem[offset - 2] = ' ';
            vidmem[offset - 1] = 0x0F;
            offset -= 2;
        }
    } else {
        vidmem[offset] = (uint8_t)c;
        vidmem[offset + 1] = attr;
        offset += 2;
    }

    if (offset >= MAX_ROWS * MAX_COLS * 2) {
        for (int i = 1; i < MAX_ROWS; ++i) {
            size_t dst = get_offset(0, i - 1);
            size_t src = get_offset(0, i);
            memcpy((uint8_t*)VIDEO_ADDRESS + dst, 
                   (uint8_t*)VIDEO_ADDRESS + src, 
                   MAX_COLS * 2);
        }
        
        size_t last_line_offset = get_offset(0, MAX_ROWS - 1);
        uint8_t *last_line = (uint8_t*)VIDEO_ADDRESS + last_line_offset;
        for (int i = 0; i < MAX_COLS; ++i) { 
            last_line[i * 2] = ' '; 
            last_line[i * 2 + 1] = 0x0f; 
        }
        offset -= 2 * MAX_COLS;
    }

    set_cursor_offset(offset);
    return offset;
}

static size_t get_cursor_offset(void) {
    port_byte_out(REG_SCREEN_CTRL, 14);
    size_t offset = (size_t)port_byte_in(REG_SCREEN_DATA) << 8; /* High byte */
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += (size_t)port_byte_in(REG_SCREEN_DATA);
    return offset * 2;
}

static void set_cursor_offset(size_t offset) {
    size_t cursor_pos = offset / 2;
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (uint8_t)(cursor_pos >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (uint8_t)(cursor_pos & 0xff));
}

void clear(void) {
    size_t screen_size = MAX_COLS * MAX_ROWS;
    volatile uint8_t *screen = VIDEO_ADDRESS;

    for (size_t i = 0; i < screen_size; ++i) {
        screen[i * 2] = ' ';
        screen[i * 2 + 1] = 0x0f;
    }
    set_cursor_offset(get_offset(0, 0));
}

static size_t get_offset(int col, int row) { 
    return (size_t)(2 * (row * MAX_COLS + col)); 
}

static int get_offset_row(size_t offset) { 
    return (int)(offset / (2 * MAX_COLS)); 
}

static int get_offset_col(size_t offset) { 
    return (int)((offset - (get_offset_row(offset) * 2 * MAX_COLS)) / 2); 
}