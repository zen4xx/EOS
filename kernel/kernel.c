#include "kernel.h"
#include "alloc.h"
#include "../libc/stdlib.h"
#include "../libc/stdio.h"
#include "../libc/vect.h"
#include "../libc/power.h"
#include "../multitasking/mt.h"
#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/gdt.h"
#include "../libc/string.h"
#include "../libc/stdint.h"

volatile char _current_char = '\0';
volatile char _is_current_task_foreground = 0;

void idle_task(void *arg)
{
    (void)arg;
    while (1) {
        task_reap();
        yield();
    }
}

void test(void *arg)
{
    char c = '0';
    int i = 0;
    char str[10];

    print("Enter your name: ");

    while(c != '\n')
    {
        c = getc();
        str[i++] = c;
    }
    str[i] = '\0';
    
    print("Hi, ");
    print(str);
}

void shell(void* arg)
{
    while (1)
    {
        if (_is_current_task_foreground == 1)
        {
            continue;
        }

        print(">");


        char str[INPUT_BUF_SIZE];
        int i = 0;
        char c = '0';

        while(c != '\n')
        {
            c = getc();

            if (c == '\b' && i > 0)
            {
                backspace(str);
                --i;
                continue;
            }

            str[i++] = c;

            if (i == INPUT_BUF_SIZE - 1)
            {
                krnl_print_at("ERR: ", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
                krnl_print("too big input\n");
                i = 0;
            }
        }
        str[i] = '\0';

        exec(str);
    }
}

void kernel_main() {

    gdt_init();
    tss_init();

	clear();

    isr_install();
    irq_install();

    init_allocator();

    krnl_print("W3lC0M3 T0 ");
	krnl_print_at("EOS\n", -1, -1, COMBINE(VGA_MAGENTA, VGA_BLACK));

    task_create_ex(idle_task, 0, 1024,0);
    task_create(shell, 0, 0);
    start_first_task();
}

void split_cmd(vect_t* vec, char* cmd);

void exec(char* cmd) {
    if (!cmd || cmd[0] == '\0' || cmd[0] == '\n') return;

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
        krnl_print_at("test", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to run test task\n");
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

    else if (strcmp(first_word, "meminfo") == 0){
        char str[32];
        itoa(malloc_info(), str);
        print(str);
        print(" bytes\n");
    }

    else if(strcmp(first_word, "test") == 0){
        task_create((void*)test, 0, 1);
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
