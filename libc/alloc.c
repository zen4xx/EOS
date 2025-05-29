#include "alloc.h"

#define ALLOCATOR_PAGE_SIZE 4096
#define NULL 0
#define ALIGNMENT 8
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

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
u32 total_allocated = 0;

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
    inital_block->prev = inital_block->next = NULL;

    free_list_head = inital_block;
    is_init = 1;
}

void* allocate(u32 size) {
    if (!is_init) init_alloc();

    
    const u32 aligned_size = ALIGN(sizeof(Block) + size);
    Block* curr = find_free_block(aligned_size);
    
    if (!curr) {
        curr = (Block*)allocate_page();
        if (!curr) return NULL;
        
        curr->size = ALLOCATOR_PAGE_SIZE;
        curr->is_free = 0;
        curr->prev = curr->next = NULL;
        curr->size = aligned_size;
    }
    
    if (curr->size > aligned_size + sizeof(Block)) {
        Block* new_block = (Block*)((char*)curr + aligned_size);
        new_block->size = curr->size - aligned_size - sizeof(Block);
        new_block->is_free = 1;
        add_to_free_list(new_block);
        curr->size = aligned_size;
    }
    
    total_allocated += curr->size;
    curr->is_free = 0;
    return (void*)(curr + 1);
}

void release(void* ptr) {
    if (!ptr) return;

    Block* block = (Block*)((char*)ptr - sizeof(Block));

    if (block->is_free) {
        err("Double free detected!\n");
        return;
    }

    total_allocated -= block->size;
    block->is_free = 1;
    
    if (block->next && block->next->is_free) {
        Block* next = block->next;
        block->size += sizeof(Block) + next->size;
        block->next = next->next;
        if (next->next) {
            next->next->prev = block;
        }
    }
    
    if (block->prev && block->prev->is_free) {
        Block* prev = block->prev;
        prev->size += sizeof(Block) + block->size;
        prev->next = block->next;
        if (block->next) {
            block->next->prev = prev;
        }
        block = prev;
    }
    
    add_to_free_list(block);
}

u32 get_allocated_size(){
    return total_allocated;
}