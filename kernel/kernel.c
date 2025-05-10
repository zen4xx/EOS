#include "kernel.h"

void kernel_main() {
	clear();
	isr_install();
	irq_install();
	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));
}

const char* hb_list[] = {"OS developer,malware???:zen4x", "Site developer,malware???:Andrew24-coop", "Fan fiction author:Yan", "Fan fiction author:oslfnkwenfm", "Fan fiction author:Kilka"};

void split_cmd(Vect* vec, char* cmd);

void exec(char* cmd){
	if(cmd){

		Vect cmds;
		init_vect(&cmds, sizeof(char*));
		
		split_cmd(&cmds, cmd);
		//stop
		if(!strcmp(cmd, "STOP")){
			krnl_print("\nstoping the cpu. Bye!\n");
			asm volatile("hlt");
		}
		//help
		else if(!strcmp(cmd, "HELP")){
			print("\nType ");krnl_print_at("STOP", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to halt the cpu");
			print("\nType ");krnl_print_at("HELP", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to print this message");
			print("\nType ");krnl_print_at("HONORBOARD", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to print honor board");
			print("\nType ");krnl_print_at("CLEAR", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to clear the screen");
			print("\nType ");krnl_print_at("CALC", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to calculate two number");
			print("\nType ");krnl_print_at("ECHO", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));print(" to print some thing");
		}
		//hb
		else if(!strcmp(cmd, "HONORBOARD")){
			for(int i = 0; i < sizeof(hb_list)/sizeof(hb_list[0]); ++i){
				krnl_print("\n");
				krnl_print_at(hb_list[i], -1, -1, i+2);
			}
		}
		//clear
		else if(!strcmp(cmd, "CLEAR")){
			clear();
		}
		//calc
		else if(!strcmp(get_vect(&cmds)[0], "CALC")){
			int a = atoi(get_vect(&cmds)[1]);
			int b = atoi(get_vect(&cmds)[3]);
			char* sign =  get_vect(&cmds)[2];

			krnl_print("\n");
			if(!strcmp(sign, "PLUS")){
				krnl_print_at(itoa(a + b), -1, -1, COMBINE(VGA_LIGHTGREEN, VGA_BLACK));
			}
			else if(!strcmp(sign, "MINUS")){
				krnl_print_at(itoa(a - b), -1, -1, COMBINE(VGA_LIGHTGREEN, VGA_BLACK));
			}
			else if(!strcmp(sign, "MULT")){
				krnl_print_at(itoa(a * b), -1, -1, COMBINE(VGA_LIGHTGREEN, VGA_BLACK));
			}
			else if(!strcmp(sign, "DIV")){
				krnl_print_at(itoa(a / b), -1, -1, COMBINE(VGA_LIGHTGREEN, VGA_BLACK));
			}
			else {
				krnl_print("Usage: CALC <num1> <OP> <num2>");
			}
		}

		else if(!strcmp(get_vect(&cmds)[0], "ECHO")){
			krnl_print("\n");
			for(int i = 1; i < get_vect_size(&cmds); ++i){
				krnl_print(get_vect(&cmds)[i]);
				krnl_print(" ");
			}
		}

		else{
			krnl_print_at("\nCommand unrecognized\n", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
			krnl_print("Type HELP to list all commands");
		}
		//cleenup	
		for (int i = 0; i < get_vect_size(&cmds); ++i) {
			char** word = (char**)get_vect(&cmds)[i];
			if (word && *word) release(*word);
		}
		delete_vect(&cmds);
	}
}

void split_cmd(Vect* vec, char* cmd) {
    if (!vec || !cmd) return;

    for (int i = 0; i < get_vect_size(vec); ++i) {
        char** old_word = (char**)get_vect(vec)[i];
        if (old_word && *old_word) {
            release(*old_word);
            *old_word = 0;
        }
    }

    vec->size = 0;

    char* start = cmd;

    while (*start != '\0') {
        while (*start != '\0' && isspace(*start)) {
            ++start;
        }

        if (*start == '\0') break;

        char* end = start;
        while (*end != '\0' && !isspace(*end)) {
            ++end;
        }

        int word_len = end - start;

        char* word = (char*)allocate(word_len + 1);
        if (!word) {
            krnl_print("Allocation failed for word\n");
            continue;
        }

        mem_cpy(word, start, word_len);
        word[word_len] = '\0';

        vect_add_elem(vec, word);

        start = end;
    }
}