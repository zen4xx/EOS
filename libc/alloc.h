#ifndef ALLOC_H
#define ALLOC_H

#include "../cpu/types.h"
#include "memory.h"
#include "error.h"

typedef struct Block {
    u32  size;          
    u8   is_free;       
    struct Block* next;
    struct Block* prev;
} Block;

void* allocate(u32 size);
void  release(void* ptr);
u32   get_total_allocated_size(void);

#endif /* ALLOC_H */
