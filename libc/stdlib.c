#include "stdlib.h"

void* malloc(u64 size) {
    register u64 num __asm__("rax") = 10;
    register u64 arg1 __asm__("rdi") = size;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1)
        : "memory", "rcx", "r11"
    );
    
    return (void*)num;
}

u64 malloc_info(void) {
    register u64 num __asm__("rax") = 13;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        :
        : "memory", "rcx", "r11"
    );
    
    return num; 
}


void free(void* ptr) {
    register u64 num __asm__("rax") = 12;
    register u64 arg1 __asm__("rdi") = (u64)ptr;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1)
        : "memory", "rcx", "r11"
    );
}

void* realloc(void* ptr, u64 size) {
    register u64 num __asm__("rax") = 11;
    register u64 arg1 __asm__("rdi") = (u64)ptr;
    register u64 arg2 __asm__("rsi") = size;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1), "r" (arg2)
        : "memory", "rcx", "r11"
    );
    
    return (void*)num;
}