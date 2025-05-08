#include "kernel.h"

void kernel_main() {
	clear();
	isr_install();
	irq_install();
	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));
}

const char* hb_list[] = {"OS developer,malware???:zen4x", "Site developer,malware???:Andrew24-coop", "Fan fiction author:Yan", "Fan fiction author:oslfnkwenfm", "Fan fiction author:Kilka"};

void exec(char* cmd){
	if(cmd){
		if(strcmp(cmd, "STOP")){
			krnl_print("\nstoping the cpu. Bye!\n");
			asm volatile("hlt");
		}
		else if(strcmp(cmd, "HELP")){
			print("\nType ");krnl_print_at("STOP", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to halt the cpu");
			print("\nType ");krnl_print_at("HELP", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to print this message");
			print("\nType ");krnl_print_at("HONORBOARD", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to print honor board");
			print("\nType ");krnl_print_at("CLEAR", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to clear the screen");
		}
		else if(strcmp(cmd, "HONORBOARD")){
			for(int i = 0; i < size(hb_list); ++i){
				krnl_print("\n");
				krnl_print_at(hb_list[i], -1, -1, i+2);
			}
		}

		else if(strcmp(cmd, "CLEAR")){
			clear();
		}

		else{
			krnl_print_at("\nCommand unrecognized\n", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
			krnl_print("Type HELP to list all commands");
		}
	}
}