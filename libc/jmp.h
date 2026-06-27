#ifndef JMP_H
#define JMP_H

#include "stdint.h"

typedef struct {
    uint32_t esp;
    uint32_t eip;
    uint32_t ebp, ebx, esi, edi;
} jmp_buf_t;

int save_here(jmp_buf_t *buf);      
__attribute__((noreturn)) void jump_to(jmp_buf_t *buf); 


#endif