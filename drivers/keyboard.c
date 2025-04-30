#include "keyboard.h"
#include "../kernel/kernel_api.h"

#define SCAN_MAX 57
const char *scan_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6", 
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", "Spacebar"};

static void keyboard_callback(registers_t regs){
    u8 scan = port_byte_in(0x60);
    if(scan > SCAN_MAX) return;
    keyboard_input(scan_name[(int)scan]);
}

void init_keyboard(){
    register_interrupt_handler(IRQ1, keyboard_callback);
}

