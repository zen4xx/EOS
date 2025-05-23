#include "keyboard.h"
#include "../kernel/kernel_api.h"

#define SCAN_MAX 57
char *scan_name[] = { "ERROR", "Esc", "1\0", "2\0", "3\0", "4\0", "5\0", "6\0", 
    "7\0", "8\0", "9\0", "0\0", "-", "=", "\b", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "\n", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", " "};

static void keyboard_callback(registers_t regs){
    u8 scan = port_byte_in(0x60);
    if(scan > SCAN_MAX) return;
    char* key = scan_name[(int)scan];
    keyboard_input(key);
}   

void init_keyboard(){
    register_interrupt_handler(IRQ1, keyboard_callback);
}

