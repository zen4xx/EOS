#include "keyboard.h"
#include "../kernel/kernel_api.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64
#define KBD_CMD    0x64

#define SCAN_MAX 57
static int shift_pressed = 0;
static int extended = 0;

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

/* Drain whatever the BIOS left sitting in the 8042 output buffer. If we
 * don't, the controller never asserts IRQ1 again and the keyboard looks
 * completely dead - a classic "works in QEMU, nothing on real hardware"
 * symptom, because QEMU hands us an empty buffer. */
static void kbd_flush(void) {
    int guard = 1024;
    while ((port_byte_in(KBD_STATUS) & 0x01) && guard--)
        (void)port_byte_in(KBD_DATA);
}

static void kbd_wait_write(void) {
    int guard = 100000;
    while ((port_byte_in(KBD_STATUS) & 0x02) && guard--)
        ;
}

static void keyboard_callback(registers_t regs) {
    (void)regs;

    if (!(port_byte_in(KBD_STATUS) & 0x01))
        return;                          /* not for us */

    u8 scancode = port_byte_in(KBD_DATA);

    /* Arrow keys, right ctrl/alt, the numpad enter etc. arrive as a 0xE0
     * prefix followed by a normal code. Without this the prefix's payload
     * was decoded as an unrelated letter. */
    if (scancode == 0xE0) { extended = 1; return; }
    if (extended)         { extended = 0; return; }

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    } else if (scancode <= SCAN_MAX) {
        char ch = shift_pressed ? scan_ascii_shift_map[scancode]
                                : scan_ascii_map[scancode];

        if (ch != '\0') {
            char str[2] = { ch, '\0' };
            keyboard_input(str);
        }
        raw_keyboard_input(ch);
    }
}

void init_keyboard(){
    register_interrupt_handler(IRQ1, keyboard_callback);

    kbd_flush();

    kbd_wait_write();
    port_byte_out(KBD_DATA, 0xF4);       /* enable scanning */
    kbd_flush();

    /* This used to be port_byte_out(0x21, 0xFD), which rewrites the whole
     * master mask and silently switched IRQ0 (and the cascade) back off. */
    irq_set_mask(1, 0);
}
