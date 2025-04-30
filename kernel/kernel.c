#include "kernel.h"

void main() {
	clear();
	isr_install();
	irq_install();
}

void exec(char* cmd){
	krnl_print(cmd);
}