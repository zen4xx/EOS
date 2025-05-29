#include "kernel.h"

void kernel_main() {

	clear();

	isr_install();
	irq_install();

	krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));
	krnl_print(">");
}

static const char* hb_list[] = {"OS developer,malware???:zen4x", "Site developer,malware???:Andrew24-coop", "Fan fiction author:Yan", "Fan fiction author:oslfnkwenfm", "Fan fiction author:Kilka"};

void split_cmd(Vect* vec, char* cmd);

void exec(char* cmd) {
    if (!cmd || cmd[0] == '\0') return;

	Vect cmds;
	init_vect(&cmds);
    split_cmd(&cmds, cmd);
    char* first_word = get_vect(&cmds)[0];

    if (strcmp(first_word, "stop") == 0) {
        krnl_print("\nStopping the CPU. Bye!\n");
        asm volatile("hlt");
    }
    else if (strcmp(first_word, "help") == 0) {
        krnl_print("\nType ");
        krnl_print_at("stop", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to halt the CPU");

        krnl_print("\nType ");
        krnl_print_at("help", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print this message");

        krnl_print("\nType ");
        krnl_print_at("honorboard", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print honor board");

        krnl_print("\nType ");
        krnl_print_at("clear", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to clear the screen");

        krnl_print("\nType ");
        krnl_print_at("calc", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to calculate two numbers");

        krnl_print("\nType ");
        krnl_print_at("echo", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print something");
    }
    else if (strcmp(first_word, "honorboard") == 0) {
        for (int i = 0; i < sizeof(hb_list)/sizeof(hb_list[0]); ++i) {
            krnl_print("\n");
            krnl_print_at(hb_list[i], -1, -1, i+2);
        }
    }
    else if (strcmp(first_word, "clear") == 0) {
        clear();
    }
    else if (strcmp(first_word, "calc") == 0) {
        if (get_vect_size(&cmds) != 4) {
            krnl_print("Usage: calc <num1> <OP> <num2>");
        } else {
            int a = atoi(get_vect(&cmds)[1]);
            int b = atoi(get_vect(&cmds)[3]);
            char* sign = get_vect(&cmds)[2];

            char res[32] = {0};
            krnl_print("\n");

            if (strcmp(sign, "plus") == 0) {
                itoa(a + b, res);
            }
            else if (strcmp(sign, "minus") == 0) {
                itoa(a - b, res);
            }
            else if (strcmp(sign, "mult") == 0) {
                itoa(a * b, res);
            }
            else if (strcmp(sign, "div") == 0) {
                if (b != 0) {
                    itoa(a / b, res);
                } else {
                    krnl_print_at("ERR", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
                }
            }
            else {
                krnl_print("Unknown operator");
            }

            if (res[0] != '\0') {
                krnl_print_at(res, -1, -1, COMBINE(VGA_LIGHTGREEN, VGA_BLACK));
            }
        }
    }
    else if (strcmp(first_word, "echo") == 0) {
        krnl_print("\n");
        for (int i = 1; i < get_vect_size(&cmds); ++i) {
            krnl_print_at(get_vect(&cmds)[i], -1, -1, COMBINE(VGA_VIOLET, VGA_BLACK));
            krnl_print(" ");
        }
    }
    else {
		krnl_print_at("\nCommand ", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print_at("\"", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print_at(first_word, -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print_at("\" unrecognized\n", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
		krnl_print("Type help to list all commands\n");
    }

	for (int i = 0; i < get_vect_size(&cmds); ++i) {
		char* word = get_vect(&cmds)[i];
		if (word) {
			release(word);
		}
	}
    delete_vect(&cmds);
}

void split_cmd(Vect* vec, char* cmd) {
    if (!vec || !cmd) return;

    char* start = cmd;
	int i = 0;
    while (*start != '\0') {
        while (*start != '\0' && isspace((unsigned char)*start)) ++start;
        if (*start == '\0') break;

        char* end = start;
        while (*end != '\0' && !isspace((unsigned char)*end)) ++end;

        int word_len = end - start;
        char* word = allocate(word_len + 1);

        if (!word) {
            krnl_print("Allocation failed for word\n");
            start = end;
            continue;
        }

        mem_cpy(word, start, word_len);
        word[word_len] = '\0'; 
        vect_add_elem(vec, word);

        start = end;
    }

    if (get_vect_size(vec) == 0) {
        krnl_print("Warning: No words were parsed.\n");
    }
}