#include "../drivers/screen.h"
#include "../cpu/isr.h"

void main() {

	clear();

	isr_install();

	__asm__ __volatile__("int $0");
	__asm__ __volatile__("int $1");
	__asm__ __volatile__("int $2");
}
