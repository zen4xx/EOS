#include "stdlib.h"

void* malloc(u32 size) 
{
    void* ptr;

    __asm__ __volatile__ (
        "movl %1, %%eax;" 
        "movl %2, %%ebx;"
        "int $0x80;"    
        "movl %%eax, %0;"
        : "=r" (ptr)    
        : "i" (10), "r" (size)  
        : "%eax", "%ebx"   
    );

    return ptr; 
}

u32 malloc_info() 
{
    u32 res;

    __asm__ __volatile__ (
        "movl %1, %%eax;" 
        "int $0x80;"    
        "movl %%eax, %0;"
        : "=r" (res)    
        : "i" (13)  
        : "%eax"   
    );

    return res; 
}

void free(void* ptr)
{
    __asm__ __volatile__ (
        "movl %0, %%eax;" 
        "movl %1, %%ebx;"
        "int $0x80;"    
        :
        : "i" (12), "r" (ptr)  
        : "%eax", "%ebx"   
    );
}

void* realloc(void* ptr, u32 size) {
    void* res_ptr;

    __asm__ __volatile__ (
        "movl %1, %%eax;"
        "movl %2, %%ebx;"
        "movl %3, %%ecx;"
        "int $0x80;"      
        : "=r" (res_ptr)   
        : "i" (11), "r" (ptr), "r" (size) 
        : "%eax", "%ebx", "%ecx" 
    );

    return res_ptr;
}