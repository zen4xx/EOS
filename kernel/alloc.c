#include "alloc.h"

#define ALLOCATOR_PAGE_SIZE (4096ULL * 1024ULL)
#define NULL_PTR ((void*)0)

/* 
 * x86-64 System V ABI requires malloc to return 16-byte aligned memory.
 */
#define ALIGNMENT 16
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

/* 
 * Start address for the heap.
 * 
 * IMPORTANT: This must be above the kernel BSS (which is at 0x100000 
 * in link.ld) and above the stage2 page tables (which are around 0x9000).
 * 4 MiB (0x400000) is a safe choice that stays within the first 1 GiB
 * identity-mapped region.
 */
#define FREE_MEM_ADDR ((void*)0x400000ULL)
#define MIN_BLOCK_SIZE (sizeof(Block) + ALIGNMENT)

static Block* free_list_head = NULL_PTR;

static void* current_free_mem_addr = FREE_MEM_ADDR;
static u8    is_init = 0;
static size_t total_allocated = 0;
static size_t num_of_pages = 0;

#define HEAP_LIMIT ((void*)0x40000000ULL) // see stage2.asm

static void* allocate_page(void) {
    if ((char*)current_free_mem_addr + ALLOCATOR_PAGE_SIZE > (char*)HEAP_LIMIT) {
        err("Heap limit reached (1 GiB)\n");
        return NULL_PTR;
    }

    void* result = current_free_mem_addr;
    current_free_mem_addr = (char*)current_free_mem_addr + ALLOCATOR_PAGE_SIZE;
    ++num_of_pages;

    return result;
}

static void add_to_free_list(Block* block) {
    block->is_free = 1;
    block->prev = NULL_PTR;
    block->next = free_list_head;
    if (free_list_head) {
        free_list_head->prev = block;
    }
    free_list_head = block;
}

static void remove_from_free_list(Block* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_list_head = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }
}

static void split_block(Block* block, size_t needed_size) {
    if (block->size < needed_size + MIN_BLOCK_SIZE) {
        return;
    }

    Block* new_block = (Block*)((char*)block + needed_size);
    new_block->size = block->size - needed_size;
    new_block->is_free = 1;
    new_block->prev = block;
    new_block->next = block->next;
    
    if (new_block->next) {
        new_block->next->prev = new_block;
    }

    block->size = needed_size - sizeof(Block); 
    block->next = new_block;

    add_to_free_list(new_block);
}

static Block* merge_with_next(Block* block) {
    if (block->next && block->next->is_free) {
        Block* nxt = block->next;
        remove_from_free_list(nxt);

        block->size += sizeof(Block) + nxt->size;
        block->next = nxt->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    return block;
}

void init_allocator(void) {
    if (is_init) {
        return;
    }

    Block* initial = (Block*)allocate_page();
    if (!initial) {
        return;
    }

    initial->size = ALLOCATOR_PAGE_SIZE - sizeof(Block);
    initial->is_free = 1;
    initial->prev = NULL_PTR;
    initial->next = NULL_PTR;

    free_list_head = initial;
    is_init = 1;
}

void* allocate(size_t size) {
    if (size == 0) {
        return NULL_PTR;
    }

    if (!is_init) {
        init_allocator();
    }

    const size_t aligned_size = ALIGN(size);
    const size_t needed_size  = aligned_size + sizeof(Block);  

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

    Block* new_block = (Block*)allocate_page();
    if (!new_block) {
        err("Failed to allocate memory: out of heap space\n");
        return NULL_PTR;
    }

    new_block->size = ALLOCATOR_PAGE_SIZE - sizeof(Block);
    new_block->is_free = 1;
    new_block->prev = NULL_PTR;
    new_block->next = NULL_PTR;

    add_to_free_list(new_block);
    
    return allocate(size);
}

void release(void* ptr) {
    if (!ptr) {
        return;
    }

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

size_t get_total_allocated_size(void) {
    return total_allocated;
}
