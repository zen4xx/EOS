#include "kernel.h"
#include "../libc/stdlib.h"
#include "../libc/stdio.h"
#include "../libc/vect.h"
#include "../libc/power.h"
#include "alloc.h"
#include "../multitasking/mt.h"

void first() {
    krnl_print("first1\n");
    mt_switch();
    krnl_print("first2\n");
    mt_switch();
    krnl_print("first3\n");
}

void second() {
    krnl_print("second1\n");
    mt_switch();
    krnl_print("second2\n");
    mt_switch();
    krnl_print("second3\n");
}

char _current_char = '\0';

void kernel_main() {

	clear();

	irq_install();
	isr_install();
    
    init_allocator();
    
    mt_init();
    mt_create_task(first);
    mt_create_task(second);
    mt_start();

	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));
	krnl_print(">");

    while(1){ 
        _current_char = '\0';
    }
    

}

static const char* hb_list[] = {"OS developer,malware???:zen4x", "Site developer,malware???:4rch1nx", "Fan fiction author:Yan", "Fan fiction author:oslfnkwenfm", "Fan fiction author:Kilka"};

void split_cmd(vect_t* vec, char* cmd);

void exec(char* cmd) {
    if (!cmd || cmd[0] == '\0') return;

	vect_t cmds;
    vect_init(&cmds, sizeof(char*));
    split_cmd(&cmds, cmd);
    char* first_word = *(char**)vect_get(&cmds, 0);

    if (strcmp(first_word, "shutdown") == 0) {
        krnl_print("Stopping the CPU. Bye!\n");
        shutdown();
    }

    else if (strcmp(first_word, "reboot") == 0) {
        krnl_print("Rebooting...\n");
        reboot();
    }

    else if (strcmp(first_word, "help") == 0) {
        krnl_print("Type ");
        krnl_print_at("shutdown", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to shutdown\n");

        krnl_print("Type ");
        krnl_print_at("reboot", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to reboot\n");

        krnl_print("Type ");
        krnl_print_at("help", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print this message\n");

        krnl_print("Type ");
        krnl_print_at("honorboard", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print honor board\n");

        krnl_print("Type ");
        krnl_print_at("clear", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to clear the screen\n");

        krnl_print("Type ");
        krnl_print_at("calc", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to calculate two numbers\n");

        krnl_print("Type ");
        krnl_print_at("echo", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print something\n");

        krnl_print("Type ");
        krnl_print_at("meminfo", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print total allocated size\n");

        krnl_print("Type ");
        krnl_print_at("animegirl", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to talk with anime waifu\n");

        krnl_print("Type ");
        krnl_print_at("blackjack", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to play blackjack\n");
    }
    else if (strcmp(first_word, "honorboard") == 0) {
        for (int i = 0; i < sizeof(hb_list)/sizeof(hb_list[0]); ++i) {
            krnl_print_at(hb_list[i], -1, -1, i+2);
            krnl_print("\n");
        }
    }
    else if (strcmp(first_word, "clear") == 0) {
        clear();
    }
    else if (strcmp(first_word, "calc") == 0) {
        if (vect_get_size(&cmds) != 4) {
            krnl_print("Usage: calc <num1> <OP> <num2>");
        } else {
            int a = atoi(*(char**)vect_get(&cmds, 1));
            int b = atoi(*(char**)vect_get(&cmds, 3));
            char* sign = *(char**)vect_get(&cmds, 2);

            char res[32] = {0};
            
            if (strcmp(sign, "+") == 0) {
                itoa(a + b, res);
            }
            else if (strcmp(sign, "-") == 0) {
                itoa(a - b, res);
            }
            else if (strcmp(sign, "*") == 0) {
                itoa(a * b, res);
            }
            else if (strcmp(sign, "/") == 0) {
                if (b != 0) {
                    itoa(a / b, res);
                } else {
                    krnl_print_at("ERR", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
                }
            }
            else {
                krnl_print_at("Calc: unknown operator", -1, -1, COMBINE(VGA_ORANGE, VGA_BLACK));
            }
            
            if (res[0] != '\0') {
                krnl_print_at(res, -1, -1, COMBINE(VGA_LIGHTGREEN, VGA_BLACK));
            }
        }
        krnl_print("\n");
    }
    else if (strcmp(first_word, "echo") == 0) {
        for (int i = 1; i < vect_get_size(&cmds); ++i) {
            krnl_print_at(*(char**)vect_get(&cmds, i), -1, -1, COMBINE(VGA_VIOLET, VGA_BLACK));
            krnl_print(" ");
        }
        krnl_print("\n");
    }

    else if(strcmp(first_word, "meminfo") == 0){
        char str[32];
        itoa(malloc_info(), str);
        print(str);
        print(" bytes\n");
    }

    else if(strcmp(first_word, "animegirl") == 0 ){
        print("nah i won't talk to u\n");
    }

    else if(strcmp(first_word, "blackjack") == 0 ){
        print("to play this game u need more than one player (u dont have friends)\n");
    }

    else {
		krnl_print_at("Command ", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print_at("\"", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print_at(first_word, -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print_at("\" unrecognized\n", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print("Type help to list all commands\n");
    }

	for (int i = 0; i < vect_get_size(&cmds); ++i) {
		char* word = *(char**)vect_get(&cmds, i);
		if (word) {
			free(word);
		}
	}
    vect_delete(&cmds);
}

void split_cmd(vect_t* vec, char* cmd) {
    if (!vec || !cmd) return;

    char* start = cmd;
    while (*start != '\0') {
        while (*start != '\0' && isspace((unsigned char)*start)) ++start;
        if (*start == '\0') break;
        
        char* end = start;
        while (*end != '\0' && !isspace((unsigned char)*end)) ++end;
        
        int word_len = end - start;
        char* word = malloc(word_len + 1);
        
        if (!word) {
            print("Allocation failed for word\n");
            start = end;
            continue;
        }
        
        memcpy(word, start, word_len);
        word[word_len] = '\0'; 
        vect_push(vec, (void*)&word);
        start = end;
    }

    if (vect_get_size(vec) == 0) {
        print("Warning: No words were parsed.\n");
    }
}
