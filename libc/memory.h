#ifndef MEMORY_H
#define MEMORY_H

#include "../cpu/types.h"

void memcpy(void* dest, void* src, u32 n);
void memset(void* dest, char c, u32 n);
void memmove(void* dest, void* src, u32 n);

#endif
