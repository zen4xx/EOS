#include "alloc.h"

#define ALLOCATOR_PAGE_SIZE 4096
#define NULL 0
#define ALIGNMENT 8
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
#define FREE_MEM_ADDR (void*)0x10000
#define MIN_BLOCK_SIZE (sizeof(Block) + 8)

static Block* free_list_head = NULL;

void* page_ptr;
u8 is_init = 0;
u32 total_allocated = 0;

void* allocate_page() {
    static void* current = FREE_MEM_ADDR;

    void* result = current;
    current += ALLOCATOR_PAGE_SIZE;
    return result;
}

void add_to_free_list(Block* block) {
    block->is_free = 1;
    block->next = free_list_head;
    block->prev = NULL;

    if (free_list_head) {
        free_list_head->prev = block;
    }

    free_list_head = block;
}

void remove_from_free_list(Block* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_list_head = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }
}

void split_block(Block* block, u32 size) {
    if (block->size >= size + MIN_BLOCK_SIZE) {
        Block* new_block = (Block*)((char*)block + size + sizeof(Block));
        new_block->size = block->size - size - sizeof(Block);
        new_block->is_free = 1;
        new_block->prev = block;
        new_block->next = block->next;

        if (new_block->next) {
            new_block->next->prev = new_block;
        }

        block->size = size;
        block->next = new_block;

        add_to_free_list(new_block);
    }
}

Block* merge_with_next(Block* block) {
    if (block->next && block->next->is_free) {
        Block* next = block->next;
        block->size += sizeof(Block) + next->size;
        block->next = next->next;

        if (next->next) {
            next->next->prev = block;
        }
    }
    return block;
}

void init_allocator() {
    if (is_init) return;

    Block* initial = (Block*)allocate_page();
    initial->size = ALLOCATOR_PAGE_SIZE - sizeof(Block);
    initial->is_free = 1;
    initial->prev = NULL;
    initial->next = NULL;

    free_list_head = initial;
    is_init = 1;
}

void* allocate(u32 size) {
    if (!is_init) init_allocator();

    const u32 aligned_size = ALIGN(size);
    const u32 needed_size = aligned_size + sizeof(Block);

    Block* curr = free_list_head;
    while (curr) {
        if (curr->is_free && curr->size >= needed_size) {
            remove_from_free_list(curr);
            curr->is_free = 0;

            if (curr->size > needed_size + MIN_BLOCK_SIZE) {
                split_block(curr, needed_size);
            }
            total_allocated += curr->size;
            return (void*)(curr + 1); 
        }
        curr = curr->next;
    }

    err("Failed to allocate memory\n");
    return NULL;
}

void release(void* ptr) {
    if (!ptr) return;

    Block* block = (Block*)ptr - 1;
    if (block->is_free) {
        err("Double free detected!\n");
        return;
    }
    total_allocated -= block->size;
    block->is_free = 1;

    if (block->next && block->next->is_free) {
        merge_with_next(block);
    }

    if (block->prev && block->prev->is_free) {
        block = merge_with_next(block->prev);
    }

    add_to_free_list(block);
}

u32 get_total_allocated_size(){
    return total_allocated;
}