#include "screen.h"
#include "ports.h"
#include "../libc/memory.h"

int get_cursor_offset();
void set_cursor_offset(int offset);
int print_char(char c, int col, int row, char attr);
int get_offset(int col, int row);
int get_offset_row(int offset);
int get_offset_col(int offset);


/**
 * Print a message on the specified location
 * If col, row, are negative, we will use the current offset 
 * white on black is 0x0f
 */
void krnl_print_at(const char *msg, int col, int row, char color) {
    int offset;
    if (col >= 0 && row >= 0)
        offset = get_offset(col, row);
    else {
        offset = get_cursor_offset();
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }

    int i = 0;
    while(msg[i] != 0){
        offset = print_char(msg[i++], col, row, color);
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }
}

void krnl_print(const char *msg) {
    krnl_print_at(msg, -1, -1, 0x0f);
}

int print_char(char c, int col, int row, char attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;
    if (!attr) attr = 0x0f;

    /* Error control: print a red 'E' if the coords aren't right */
    if (col >= MAX_COLS || row >= MAX_ROWS) {
        vidmem[2*(MAX_COLS)*(MAX_ROWS)-2] = 'E';
        vidmem[2*(MAX_COLS)*(MAX_ROWS)-1] = 0x04;
        return get_offset(col, row);
    }

    int offset;
    if (col >= 0 && row >= 0) offset = get_offset(col, row);
    else offset = get_cursor_offset();

    if (c == '\n') {
        row = get_offset_row(offset);
        offset = get_offset(0, row+1);
    } 
    else if(c == '\b'){
        vidmem[offset-2] = ' ';
        vidmem[offset-1] = 0x0F;
        offset -= 2;
    }
    else {
        vidmem[offset] = c;
        vidmem[offset+1] = attr;
        offset += 2;
    }

    if (offset >= MAX_ROWS * MAX_COLS * 2){
    	for(int i = 0; i < MAX_ROWS; ++i){
		mem_cpy(get_offset(0, i-1) + VIDEO_ADDRESS, 
				get_offset(0, i) + VIDEO_ADDRESS,
				MAX_COLS * 2); 
	}
	char* last_line = get_offset(0, MAX_ROWS - 1) + VIDEO_ADDRESS;
	for(int i = 0; i < MAX_COLS * 2; ++i) last_line[i] = 0;
	offset -= 2 * MAX_COLS;
    }

    set_cursor_offset(offset);
    return offset;
}

int get_cursor_offset() {
    port_byte_out(REG_SCREEN_CTRL, 14);
    int offset = port_byte_in(REG_SCREEN_DATA) << 8; /* High byte: << 8 */
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += port_byte_in(REG_SCREEN_DATA);
    return offset * 2; /* Position * size of character cell */
}

void set_cursor_offset(int offset) {
    offset /= 2;
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset & 0xff));
}

void clear() {
    int screen_size = MAX_COLS * MAX_ROWS;
    int i;
    char *screen = VIDEO_ADDRESS;

    for (i = 0; i < screen_size; ++i) {
        screen[i*2] = ' ';
        screen[i*2+1] = 0x0f;
    }
    set_cursor_offset(get_offset(0, 0));
}


int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }
int get_offset_row(int offset) { return offset / (2 * MAX_COLS); }
int get_offset_col(int offset) { return (offset - (get_offset_row(offset)*2*MAX_COLS))/2; }
