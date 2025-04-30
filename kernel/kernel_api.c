#include "kernel_api.h"

char* input_msg = "";

//text appears after enter

void keyboard_input(char c[]){
    //later...
    if(strcmp(c, "Esc")){
        krnl_print("stoping the cpu\n");
        asm volatile("hlt");
    }
    
    else if(strcmp(c, "Enter")){
        exec(input_msg);
        input_msg[0] = '\0';
        krnl_print("\n");
    }

    else if(strcmp(c, "Backspace") && strlen(input_msg)) backspace(input_msg);
    else if(strcmp(c, "Spacebar")) append(input_msg, ' ');
    else 
    {
        append_str(input_msg, c);
    }
}