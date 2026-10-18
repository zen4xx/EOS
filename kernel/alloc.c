#include "alloc.h"

#define ALLOCATOR_PAGE_SIZE (4096ULL * 1024ULL)
#define NULL_PTR ((void*)0)
#define ALIGNMENT 16
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define FREE_MEM_ADDR ((void*)0x400000ULL)
#define MIN_BLOCK_SIZE (sizeof(Block) + ALIGNMENT)
#define MAGIC_1 0x200325 // echobreak
#define MAGIC_2 0x240125
#define MAGIC_3 0x151225

const u32 magic = MAGIC_1 ^ MAGIC_2 ^ MAGIC_3;

static Block* free_list_head = NULL_PTR;
static void* current_free_mem_addr = FREE_MEM_ADDR;
static u8 is_init = 0;
static size_t total_allocated = 0;
static size_t num_of_pages = 0;
#define HEAP_LIMIT ((void*)0x40000000ULL)

static void link_phys_blocks(Block* b1, Block* b2) {
    if (b1 && b2) {
        b1->phys_next = b2;
        b2->phys_prev = b1;
    }
}

static void insert_into_free_list(Block* block) {
    block->is_free = 1;
    block->free_prev = NULL_PTR;
    block->free_next = free_list_head;
    if (free_list_head) {
        free_list_head->free_prev = block;
    }
    free_list_head = block;
}

static void remove_from_free_list(Block* block) {
    if (block->free_prev) {
        block->free_prev->free_next = block->free_next;
    } else {
        free_list_head = block->free_next;
    }
    if (block->free_next) {
        block->free_next->free_prev = block->free_prev;
    }
    block->free_prev = NULL_PTR;
    block->free_next = NULL_PTR;
}

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

static void split_block(Block* block, size_t needed_size) {
    if (block->size < needed_size + MIN_BLOCK_SIZE) {
        return;
    }

    Block* new_block = (Block*)((char*)block + needed_size);
    
    new_block->size = block->size - needed_size;
    new_block->magic = magic;
    
    Block* old_phys_next = block->phys_next;
    
    link_phys_blocks(block, new_block);
    link_phys_blocks(new_block, old_phys_next);

    block->size = needed_size; 

    insert_into_free_list(new_block);
}

static Block* merge_with_next(Block* block) {
    Block* nxt = block->phys_next;
    if (nxt && nxt->is_free) {
        remove_from_free_list(nxt);
        
        block->size += nxt->size;
        
        Block* after_nxt = nxt->phys_next;
        block->phys_next = after_nxt;
        if (after_nxt) {
            after_nxt->phys_prev = block;
        }
    }
    return block;
}

void init_allocator(void) {
    if (is_init) return;

    Block* initial = (Block*)allocate_page();
    if (!initial) return;

    initial->size = ALLOCATOR_PAGE_SIZE;
    initial->is_free = 1;
    initial->magic = magic;
    initial->phys_prev = NULL_PTR;
    initial->phys_next = NULL_PTR;
    initial->free_prev = NULL_PTR;
    initial->free_next = NULL_PTR;

    free_list_head = initial;
    is_init = 1;
}

void* allocate(size_t size) {
    if (size == 0) return NULL_PTR;
    if (!is_init) init_allocator();

    const size_t aligned_size = ALIGN(size);
    const size_t needed_size = aligned_size + sizeof(Block);

    Block* curr = free_list_head;
    while (curr) {
        if (curr->size >= needed_size) {
            remove_from_free_list(curr);
            curr->is_free = 0;
            curr->magic = magic;

            if (curr->size > needed_size + MIN_BLOCK_SIZE) {
                split_block(curr, needed_size);
            }

            total_allocated += curr->size;
            return (void*)(curr + 1);
        }
        curr = curr->free_next;
    }

    Block* new_block = (Block*)allocate_page();
    if (!new_block) {
        err("Failed to allocate memory: out of heap space\n");
        return NULL_PTR;
    }

    new_block->size = ALLOCATOR_PAGE_SIZE;
    new_block->is_free = 1;
    new_block->magic = magic;
    new_block->phys_prev = NULL_PTR;
    new_block->phys_next = NULL_PTR;
    new_block->free_prev = NULL_PTR;
    new_block->free_next = NULL_PTR;

    insert_into_free_list(new_block);

    return allocate(size); 
}

void release(void* ptr) {
    if (!ptr) return;

    Block* block = (Block*)ptr - 1;
    
    if (block->is_free) {
        err("Double free detected!\n");
        return;
    }

    if (block->magic != magic) {
        err("Segmentation fault (Invalid magic)\n");
        return;
    }

    total_allocated -= block->size;
    block->is_free = 1;

    merge_with_next(block);

    if (block->phys_prev && block->phys_prev->is_free) {
        remove_from_free_list(block->phys_prev);
        block->phys_prev->size += block->size;
        block->phys_prev->phys_next = block->phys_next;
        if (block->phys_next) {
            block->phys_next->phys_prev = block->phys_prev;
        }
        block = block->phys_prev;
    }

    insert_into_free_list(block);
}

size_t get_total_allocated_size(void) {
    return total_allocated;
}