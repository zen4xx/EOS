#include "stdio.h"
#include "../syscall/syscall.h"

void print(char* msg){
    __asm__ (
        "movl %0, %%ebx" 
        :                 
        : "r" (msg) 
        : "%ebx"          
    );
    __asm__ ("movl $1, %eax");
    __asm__ __volatile__ ("int $0x80"); //syscall
}

char getc()
{
    return '0'; //later
}
