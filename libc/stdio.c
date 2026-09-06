#include "stdio.h"

/* This was three separate asm statements. Nothing stopped GCC from putting
 * its own code (or a reload of msg) between them and clobbering ebx before
 * the int 0x80 ran - it just happened not to at -O0. One block, explicit
 * constraints. */
void print(const char* msg){
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a" (1), "b" (msg)
        : "memory"
    );
}


char getc() {

    char c;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a" (c)
        : "a" (30)          // syscall num
        : "memory"
    );

    return c; 
}
