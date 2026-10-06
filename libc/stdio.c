#include "stdio.h"
#include "stdint.h"

void print(const char* msg) {
    register uint64_t num __asm__("rax") = 1;
    register uint64_t arg1 __asm__("rdi") = (uint64_t)msg;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1)
        : "memory", "rcx", "r11"
    );
}

char getc(void) {
    register uint64_t num __asm__("rax") = 30;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        :
        : "memory", "rcx", "r11"
    );
    
    return (char)num;  // return value is in rax
}