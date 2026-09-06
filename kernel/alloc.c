#include "alloc.h"

#define ALLOCATOR_PAGE_SIZE 8192
#define NULL 0
#define ALIGNMENT 8
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
/* The heap used to sit at 0x10000, which is only 60 KB below the BIOS/EBDA
 * area at 0x9FC00 - a few pages of allocation and it walks straight into
 * firmware memory. Now that stage 2 enables A20 we can use real RAM above
 * 1 MB instead. HEAP_LIMIT is deliberately conservative (8 MB); any machine
 * that can run an i5-12400F has far more than that. */
#define FREE_MEM_ADDR  (void*)0x00100000
#define HEAP_LIMIT     (void*)0x00800000
#define MIN_BLOCK_SIZE (sizeof(Block) + 8)

static Block* free_list_head = NULL;

void* page_ptr;
u8 is_init = 0;
u32 total_allocated = 0;

void* current_free_mem_addr = FREE_MEM_ADDR;

static u32 num_of_pages = 0;

void* allocate_page() {
    if ((char*)current_free_mem_addr + ALLOCATOR_PAGE_SIZE > (char*)HEAP_LIMIT)
        return NULL;

    void* result = current_free_mem_addr;
    current_free_mem_addr = (char*)current_free_mem_addr + ALLOCATOR_PAGE_SIZE;

    ++num_of_pages;

    return result;
}

void add_to_free_list(Block* block) {
    block->is_free = 1;
    block->prev = NULL;
    block->next = free_list_head;
    if (free_list_head) free_list_head->prev = block;
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

void split_block(Block* block, u32 needed_size) {
    if (block->size < needed_size + MIN_BLOCK_SIZE) return;
    Block* new_block = (Block*)((char*)block + needed_size);
    new_block->size = block->size - needed_size - sizeof(Block);
    new_block->is_free = 1;
    new_block->prev = block;
    new_block->next = block->next;
    if (new_block->next) new_block->next->prev = new_block;

    block->size = needed_size - sizeof(Block); 
    block->next = new_block;

    add_to_free_list(new_block);
}


Block* merge_with_next(Block* block) {
    if (block->next && block->next->is_free) {
        Block* nxt = block->next;
        remove_from_free_list(nxt);

        block->size += sizeof(Block) + nxt->size;
        block->next = nxt->next;
        if (block->next) block->next->prev = block;
    }
    return block;
}

void init_allocator() {
    if (is_init) return;
    Block* initial = (Block*)allocate_page();
    if (!initial) return;
    initial->size = ALLOCATOR_PAGE_SIZE - sizeof(Block);
    initial->is_free = 1;
    initial->prev = NULL;
    initial->next = NULL;

    free_list_head = initial;
    is_init = 1;
}

void* allocate(u32 size) {
    const u32 aligned_size = ALIGN(size);
    const u32 needed_size  = aligned_size + sizeof(Block);  

    if ((ALLOCATOR_PAGE_SIZE * num_of_pages) - total_allocated <= size)
    {
        Block* new_allocated_block = (Block*)allocate_page();
        if (new_allocated_block) {
            new_allocated_block->size = ALLOCATOR_PAGE_SIZE - sizeof(Block);
            add_to_free_list(new_allocated_block);
        }
    } 

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
