#ifndef MEMORY_H
#define MEMORY_H

#include "../cpu/types.h"

void mem_cpy(void* dest, void* src, u32 n);
void mem_set(void* dest, char c, u32 n);

#endif
