#include "progs.h"
#include "kernel.h"
#include "../libc/stdlib.h"
#include "../libc/stdio.h"
#include "../libc/vect.h"
#include "../multitasking/mt.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/stdint.h"
#include "../fs/vfs.h"
#include "../libc/stdlib.h"
#include "../libc/stdio.h"
#include "../libc/vect.h"
#include "../libc/power.h"


void idle_task(int argc, char **argv)
{
    while (1) {
        task_reap();
        yield();
    }
}

void test(int argc, char **argv)
{
    char c = '0';
    int i = 0;
    char str[10];

    if (argc > 0)
    {
        print("argv:\n");
        for (int i = 0; i < argc; ++i)
        {
            char istr[10];
            itoa(i, istr);
            print(istr);
            print(": ");
            print(argv[i]);
            print("\n");
        }
    }

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

void ls(int argc, char **argv)
{
    dirent_t dirent;
    uint32_t index = 0;

    while (readdir(".", index, &dirent) == 0) {
        dirent.name[VFS_MAX_NAME - 1] = '\0';

        print(dirent.name);

        if (dirent.flags & VFS_FLAG_DIRECTORY)
            print("/");

        print("  ");

        index++;
    }

    print("\n");
}

void write(int argc, char **argv)
{
    if (argc < 3 || !argv[1] || !argv[2]) {
        print("usage: write <file> <text>\n");
        return;
    }

    int file = fopen(argv[1], O_WRONLY | O_CREAT | O_TRUNC);

    if (file < 0) {
        print("write: cannot open file\n");
        return;
    }

    for (int i = 2; i < argc; ++i)
    {
        uint32_t len = (uint32_t)strlen(argv[i]);
        int written = fwrite(file, argv[i], len);

        fwrite(file, " ", 1);

        if (written < 0 || (uint32_t)written != len) {
            print("write: write error\n");
            fclose(file);
            return;
        }
    }

    fclose(file);
}

void cat(int argc, char **argv)
{
    if (argc < 2 || !argv[1]) {
        print("usage: cat <file>\n");
        return;
    }

    int file = fopen(argv[1], O_RDONLY);

    if (file < 0) {
        print("cat: cannot open file\n");
        return;
    }

    char buf[257];

    while (1) {
        int bytes = fread(file, buf, sizeof(buf) - 1);

        if (bytes < 0) {
            print("cat: read error\n");
            break;
        }

        if (bytes == 0)
            break;

        buf[bytes] = '\0';
        print(buf);
    }

    print("\n");

    fclose(file);
}

void cd(int argc, char **argv)
{
    if (argc < 2 || !argv[1])
    {
        print("usage: cd <directory>\n");
        return;
    }

    change_cwd(argv[1]);
}

void mkdir_cmd(int argc, char **argv)
{
    if (argc < 2 || !argv[1]) {
        print("usage: mkdir <directory>\n");
        return;
    }

    if (mkdir(argv[1]) != 0)
        print("mkdir: cannot create directory\n");
    
}

void shell(int argc, char **argv)
{
    while (1)
    {
        if (_is_current_task_foreground == 1)
        {
            continue;
        }

        char buf[256];
        get_cwd(buf, 256);
        print("[ ");
        print(buf);
        print(" ]>");


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
                str[i] = '\0';
                krnl_print_at("ERR: ", -1, -1, COMBINE(VGA_RED, VGA_BLACK));
                krnl_print("too big input\n");
                i = 0;
            }
        }
        str[i] = '\0';

        exec(str);
    }
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
        
        krnl_print("Type ");
        krnl_print_at("cd", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to change directory\n");
        
        krnl_print("Type ");
        krnl_print_at("mkdir", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to make dir\n");
        
        krnl_print("Type ");
        krnl_print_at("ls", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print files and directories at current directory\n");
        
        krnl_print("Type ");
        krnl_print_at("cat", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to print file data\n");

        krnl_print("Type ");
        krnl_print_at("write", -1, -1, COMBINE(VGA_YELLOW, VGA_BLACK));
        krnl_print(" to write data to file\n");
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
        int argc = vect_get_size(&cmds);
        if (argc > 1)
            task_create((void*)test, vect_get_size(&cmds), (char**)cmds.arr, 1, 1);
        else
            task_create((void*)test, 0, 0, 1, 1); 
    }
        
    else if(strcmp(first_word, "ls") == 0){
        if (vect_get_size(&cmds) > 1)
            task_create(ls, vect_get_size(&cmds), (char**)cmds.arr, 1, 1);
        else 
            task_create(ls, 0, 0, 1, 1);
    }

    else if(strcmp(first_word, "write") == 0){
        if (vect_get_size(&cmds) > 1)
            task_create(write, vect_get_size(&cmds), (char**)cmds.arr, 1, 1);
        else 
            task_create(write, 0, 0, 1, 1);
    }
    
    else if(strcmp(first_word, "cat") == 0){
        if (vect_get_size(&cmds) > 1)
            task_create(cat, vect_get_size(&cmds), (char**)cmds.arr, 1, 1);
        else 
            task_create(cat, 0, 0, 1, 1);
    }

    else if(strcmp(first_word, "cd") == 0){
        if (vect_get_size(&cmds) > 1)
            task_create(cd, vect_get_size(&cmds), (char**)cmds.arr, 1, 1);
        else 
            task_create(cd, 0, 0, 1, 1);
    }

    else if(strcmp(first_word, "mkdir") == 0){
        if (vect_get_size(&cmds) > 1)
            task_create(mkdir_cmd, vect_get_size(&cmds), (char**)cmds.arr, 1, 1);
        else 
            task_create(mkdir_cmd, 0, 0, 1, 1);
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
