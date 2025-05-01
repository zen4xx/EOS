#include "kernel.h"

void main() {
	clear();
	isr_install();
	irq_install();
	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, 0x05);
	krnl_print("type stop to halt the cpu\n");
}

void exec(char* cmd){
	if(strcmp(cmd, "STOP")){
		krnl_print("\nstoping the cpu. Bye!\n");
		asm volatile("hlt");
	}
	else if(strcmp(cmd, "HELP")){
		krnl_print("\nComing soon!\n");
	}
}
