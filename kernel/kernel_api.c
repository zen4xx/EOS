#include "kernel_api.h"

char input_msg[INPUT_BUF_SIZE] = "";

void keyboard_input(char c[]){
    if(c[0] == '\n'){
        krnl_print(c);
        exec(input_msg);
        input_msg[0] = '\0';
        krnl_print(">");
    }
    else if(c[0] == '\b'){
        if(strlen(input_msg) > 0){
            backspace(input_msg);
            krnl_print(c);
        }
    }
    else{
        append_str(input_msg, c);
        krnl_print(c);
    }
}

void raw_keyboard_input(char c) { _current_char = c; };
