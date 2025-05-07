#include "kernel_api.h"

char* input_msg = "";

void keyboard_input(char c[]){

    if(c[0] == '\n'){
        exec(input_msg);
        input_msg[0] = '\0';
        krnl_print(c);
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
