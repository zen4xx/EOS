#include "jmp.h"

__attribute__((naked)) int save_here(jmp_buf_t *buf) {
    __asm__ volatile(
        "movl 4(%esp), %eax\n\t"   
        "movl %esp,    0(%eax)\n\t"
        "movl 0(%esp), %edx\n\t"   
        "movl %edx,    4(%eax)\n\t"
        "movl %ebp,    8(%eax)\n\t"
        "movl %ebx,   12(%eax)\n\t"
        "movl %esi,   16(%eax)\n\t"
        "movl %edi,   20(%eax)\n\t"
        "xorl %eax, %eax\n\t"      
        "ret\n\t"
    );
}

__attribute__((naked,noreturn)) void jump_to(jmp_buf_t *buf) {
    __asm__ volatile(
        "movl 4(%esp), %ecx\n\t"   
        "movl 0(%ecx), %esp\n\t"
        "movl 8(%ecx), %ebp\n\t"
        "movl 12(%ecx), %ebx\n\t"
        "movl 16(%ecx), %esi\n\t"
        "movl 20(%ecx), %edi\n\t"
        "movl $1, %eax\n\t"
        "jmp  *4(%ecx)\n\t"        
    );
}
