#include "kernel.h"

#define HB

void kernel_main() {
	clear();
	isr_install();
	irq_install();
	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));
}

char* hb_list[] = {"OS developer,malware???:zen4x", "Site developer,malware???:Andrew24-coop", "Fan fiction author:Yan", "Fan fiction author:oslfnkwenfm", "Fan fiction author:Kilka"};

char* get_word(char* cmd, int index); //it will be later split and returns array of string

void exec(char* cmd){
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


char* get_word(char* cmd, int index){
	char* word = "\0";
	int len = strlen(cmd);
	int j = 0;
	for(int i = 0; i < len; ++i){
		if(cmd[i] == ' '){
			if(j == index) return word;
			word[0] = '\0';
			++j;
		}
		else {
			append(word, cmd[i]); 
		}
	}
	if(index == j) return word;
	return "";
}