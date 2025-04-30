#include "keyboard.h"

void print_letter(u8 scan);

static void keyboard_callback(registers_t regs){
    u8 scan = port_byte_in(0x60);
    char *sc_ascii;
    int_to_ascii(scan, sc_ascii);
    krnl_print("Keyboard: ");
    krnl_print(sc_ascii);
    krnl_print(", ");
    print_letter(scan);
    krnl_print("\n");
}

void init_keyboard(){
    register_interrupt_handler(IRQ1, keyboard_callback);
}

void print_letter(u8 scan) {
    switch (scan) {
        case 0x0:
            krnl_print("ERROR");
            break;
        case 0x1:
            krnl_print("ESC");
            break;
        case 0x2:
            krnl_print("1");
            break;
        case 0x3:
            krnl_print("2");
            break;
        case 0x4:
            krnl_print("3");
            break;
        case 0x5:
            krnl_print("4");
            break;
        case 0x6:
            krnl_print("5");
            break;
        case 0x7:
            krnl_print("6");
            break;
        case 0x8:
            krnl_print("7");
            break;
        case 0x9:
            krnl_print("8");
            break;
        case 0x0A:
            krnl_print("9");
            break;
        case 0x0B:
            krnl_print("0");
            break;
        case 0x0C:
            krnl_print("-");
            break;
        case 0x0D:
            krnl_print("+");
            break;
        case 0x0E:
            krnl_print("Backspace");
            break;
        case 0x0F:
            krnl_print("Tab");
            break;
        case 0x10:
            krnl_print("Q");
            break;
        case 0x11:
            krnl_print("W");
            break;
        case 0x12:
            krnl_print("E");
            break;
        case 0x13:
            krnl_print("R");
            break;
        case 0x14:
            krnl_print("T");
            break;
        case 0x15:
            krnl_print("Y");
            break;
        case 0x16:
            krnl_print("U");
            break;
        case 0x17:
            krnl_print("I");
            break;
        case 0x18:
            krnl_print("O");
            break;
        case 0x19:
            krnl_print("P");
            break;
		case 0x1A:
			krnl_print("[");
			break;
		case 0x1B:
			krnl_print("]");
			break;
		case 0x1C:
			krnl_print("ENTER");
			break;
		case 0x1D:
			krnl_print("LCtrl");
			break;
		case 0x1E:
			krnl_print("A");
			break;
		case 0x1F:
			krnl_print("S");
			break;
        case 0x20:
            krnl_print("D");
            break;
        case 0x21:
            krnl_print("F");
            break;
        case 0x22:
            krnl_print("G");
            break;
        case 0x23:
            krnl_print("H");
            break;
        case 0x24:
            krnl_print("J");
            break;
        case 0x25:
            krnl_print("K");
            break;
        case 0x26:
            krnl_print("L");
            break;
        case 0x27:
            krnl_print(";");
            break;
        case 0x28:
            krnl_print("'");
            break;
        case 0x29:
            krnl_print("`");
            break;
		case 0x2A:
			krnl_print("LShift");
			break;
		case 0x2B:
			krnl_print("\\");
			break;
		case 0x2C:
			krnl_print("Z");
			break;
		case 0x2D:
			krnl_print("X");
			break;
		case 0x2E:
			krnl_print("C");
			break;
		case 0x2F:
			krnl_print("V");
			break;
        case 0x30:
            krnl_print("B");
            break;
        case 0x31:
            krnl_print("N");
            break;
        case 0x32:
            krnl_print("M");
            break;
        case 0x33:
            krnl_print(",");
            break;
        case 0x34:
            krnl_print(".");
            break;
        case 0x35:
            krnl_print("/");
            break;
        case 0x36:
            krnl_print("Rshift");
            break;
        case 0x37:
            krnl_print("Keypad *");
            break;
        case 0x38:
            krnl_print("LAlt");
            break;
        case 0x39:
            krnl_print("Spc");
            break;
        default:
            if (scan <= 0x7f) {
                krnl_print("Unknown key down");
            } else if (scan <= 0x39 + 0x80) {
                krnl_print("key up ");
                print_letter(scan - 0x80);
            } else krnl_print("Unknown key up");
            break;
    }
}