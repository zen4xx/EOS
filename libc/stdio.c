#include "stdio.h"

void print(void* msg){
    krnl_print((char*)msg);
}

char getc()
{
    while(_current_char == '\0'); 
    return _current_char;
}
