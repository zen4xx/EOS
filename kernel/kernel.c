#include "kernel.h"

#define HB

void kernel_main() {
	clear();
	isr_install();
	irq_install();
	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));
}

char* hb_list[] = {"OS developer:zen4x", "Site developer:Anrew24-coop", "Fan fiction author:Yan", "Fan fiction author:oslfnkwenfm", "Fan fiction author:Kilka"};

void exec(char* cmd){
	if(strcmp(cmd, "STOP")){
		krnl_print("\nstoping the cpu. Bye!\n");
		asm volatile("hlt");
	}
	else if(strcmp(cmd, "HELP")){
		print("\nType STOP to halt the cpu");
		print("\nType HELP to print this message");
		print("\nType HONORBOARD to print honor board");
	}
	else if(strcmp(cmd, "HONORBOARD")){
		for(int i = 0; i < sizeof(hb_list)/(sizeof(char*)); ++i){
			krnl_print("\n");
			krnl_print_at(hb_list[i], -1, -1, i+2);
		}
	}
	else{
		krnl_print_at("\nCommand unrecognized\n", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print("Type HELP to list all commands");
	}
}
