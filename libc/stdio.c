#include "stdio.h"

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


char getc() {

    char c;
    __asm__ __volatile__ (
        "movl %1, %%eax;"
        "int $0x80;"      
        "movb %%al, %0;"   
        : "=r" (c)          
        : "i" (30)  // syscall num
        : "%eax" 
    );

    return c; 
}
