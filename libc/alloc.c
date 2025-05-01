#include "alloc.h"
#include  "../cpu/types.h"

#define ALLOCATOR_PAGE_SIZE 4096
#define NULL 0

typedef struct Block{
    int is_free;
    int size;
    struct Block* next;
    struct Block* prev;
}Block;

static Block* free_list_head = NULL;

void* page_ptr;
u8 is_init = 0;
void* free_mem_addr = (void*)0x10000;

void* allocate_page(){
    free_mem_addr += ALLOCATOR_PAGE_SIZE;
    return free_mem_addr;
}


void add_to_free_list(Block* block){
    if(!free_list_head){
        free_list_head = block;
        block->prev = block->next = NULL;
    }
    else{
        block->next = free_list_head;
        block->prev = NULL;
        free_list_head->prev = block;
        free_list_head = block;
    }
} 

void remove_from_free_list(Block* block){
    if(block->prev) block->prev->next = block->next;
    if(block->next) block->next->prev = block->prev;
    if(block == free_list_head) free_list_head = block->next;
}

Block* find_free_block(int size){
    Block* curr = free_list_head;
    while(curr){
        if(curr->size >= size){
            remove_from_free_list(curr);
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void init_alloc(){
    page_ptr = allocate_page();
    Block* inital_block = (Block*)page_ptr;
    inital_block->size = ALLOCATOR_PAGE_SIZE;
    inital_block->is_free = 1;
}

void* allocate(int size){
    if(!is_init) init_alloc();
    int total_size = sizeof(Block) + size + (ALLOCATOR_PAGE_SIZE - 1);
    total_size &= ~(ALLOCATOR_PAGE_SIZE-1);
    Block* curr = find_free_block(total_size);
    if(!curr){
        curr = (Block*)allocate_page();
        curr->size = ALLOCATOR_PAGE_SIZE;
        curr->is_free = 0;
    }
    if(curr->size > total_size){
        Block* new_block = (Block*)((char*)curr + total_size);
        new_block->size = curr->size - total_size - sizeof(Block);
        new_block->is_free = 1;
        add_to_free_list(new_block);
    }
    curr->is_free = 0;
    return(void*)(curr + 1);
}


void release(void* ptr){
    if(!ptr) return;
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->is_free = 1;

    if(block->prev && block->prev->is_free){
        Block* prev_block = block->prev;
        prev_block->size += block->size + sizeof(Block);
        if(block->next) block->next->prev = prev_block;
        block = prev_block;
    }
    if(block->next && block->next->is_free){
        Block* next_block = block->next;
        block->size += next_block->size + sizeof(Block);
        block->next = next_block->next;
        if(next_block->next) next_block->next->prev = block;
        block = next_block;
    }
    add_to_free_list(block);
}