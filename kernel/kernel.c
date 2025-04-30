#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../libc/libc.h"
#include "../drivers/keyboard.h"

void main() {

	clear();

	isr_install();

	asm volatile("sti");
	init_timer(50);

	init_keyboard();

}
