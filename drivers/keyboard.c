#include "keyboard.h"
#include "../kernel/kernel_api.h"

#define SCAN_MAX 57
static int shift_pressed = 0;

char scan_ascii_map[SCAN_MAX + 1] = {
    0,   0,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u',
    'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's', 'd', 'f',
    'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\','z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*',
    0,   ' '
};

char scan_ascii_shift_map[SCAN_MAX + 1] = {
    0,   0,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',
    'I', 'O', 'P', '{', '}', '\n', 0,   'A', 'S', 'D', 'F',
    'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*',
    0,   ' '
};

static void keyboard_callback(registers_t regs) {
    (void)regs; 

    u8 scancode = port_byte_in(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1; 
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0; 
    } else if (scancode <= SCAN_MAX) {
        char ch = '\0';

        if (shift_pressed)
            ch = scan_ascii_shift_map[scancode];
        else
            ch = scan_ascii_map[scancode];

        if (ch != '\0') {
            char str[2] = { ch, '\0' };
            keyboard_input(str);
        }
    }
} 

void init_keyboard(){
    register_interrupt_handler(IRQ1, keyboard_callback);
    port_byte_out(0x21, 0xFD);
}

