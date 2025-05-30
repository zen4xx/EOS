#ifndef ALLOC_H
#define ALLOC_H
#include "../cpu/types.h"
#include "memory.h"
#include "error.h"

void* allocate(u32 size);
void release(void* ptr);
u32 get_total_allocated_size();

typedef struct Block{
    int is_free;
    int size;
    struct Block* next;
    struct Block* prev;
}Block;

#endif