#include "alloc.h"
#include "pmm.h"
#include "paging.h"
#include "irqflags.h"

#define NULL_PTR ((void*)0)
#define ALIGNMENT 16
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define HEAP_BASE  0x40000000ULL              /* 1 GiB, virtual */
#define HEAP_LIMIT 0x80000000ULL              /* 2 GiB, virtual */
#define HEAP_FLAGS (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)

#define CHUNK_PAGES 64                        /* 256 KiB default chunk */
#define CHUNK_SIZE  ((u64)CHUNK_PAGES * PMM_PAGE_SIZE)

#define MIN_BLOCK_SIZE (sizeof(Block) + ALIGNMENT)
#define MAGIC_1 0x200325 // echobreak
#define MAGIC_2 0x240125
#define MAGIC_3 0x151225
#define CHUNK_MAGIC 0x454F5343u               /* "EOSC" */

static const u32 magic = MAGIC_1 ^ MAGIC_2 ^ MAGIC_3;

typedef struct Chunk {
    u32 magic;
    u32 pages;
    u32 used_blocks;
    u32 pad;
    struct Chunk* prev;
    struct Chunk* next;
    size_t arena_size;
} __attribute__((aligned(16))) Chunk;

static Block* free_list_head = NULL_PTR;
static Chunk* chunk_list = NULL_PTR;
static u64    heap_top = HEAP_BASE;
static u8     is_init = 0;
static size_t total_allocated = 0;
static size_t num_of_pages = 0;

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

static Chunk* grow_heap(size_t min_bytes) {
    size_t need = ALIGN(sizeof(Chunk)) + min_bytes;
    u64 pages = CHUNK_PAGES;
    if (need > CHUNK_SIZE) {
        pages = (u64)((need + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE);
    }
    u64 bytes = pages * PMM_PAGE_SIZE;

    if (heap_top + bytes > HEAP_LIMIT) {
        err("Heap limit reached (1 GiB of virtual space)\n");
        return NULL_PTR;
    }
    if (paging_alloc_map(heap_top, pages, HEAP_FLAGS) != 0) {
        err("Heap grow failed: out of physical memory\n");
        return NULL_PTR;
    }

    Chunk* c = (Chunk*)heap_top;
    c->magic = CHUNK_MAGIC;
    c->pages = (u32)pages;
    c->used_blocks = 0;
    c->pad = 0;
    c->arena_size = (size_t)(bytes - ALIGN(sizeof(Chunk)));
    c->prev = NULL_PTR;
    c->next = chunk_list;
    if (chunk_list) chunk_list->prev = c;
    chunk_list = c;

    Block* b = (Block*)((char*)c + ALIGN(sizeof(Chunk)));
    b->size = c->arena_size;
    b->magic = magic;
    b->phys_prev = NULL_PTR;
    b->phys_next = NULL_PTR;
    b->chunk = c;
    insert_into_free_list(b);

    heap_top += bytes;
    num_of_pages += (size_t)pages;
    return c;
}

static void free_chunk(Chunk* c) {
    if (c->prev) c->prev->next = c->next;
    else         chunk_list = c->next;
    if (c->next) c->next->prev = c->prev;

    if ((u64)c + (u64)c->pages * PMM_PAGE_SIZE == heap_top) {
        heap_top = (u64)c;
    }
    num_of_pages -= c->pages;
    paging_free_unmap((u64)c, c->pages);
}

static void split_block(Block* block, size_t needed_size) {
    if (block->size < needed_size + MIN_BLOCK_SIZE) {
        return;
    }
    Block* new_block = (Block*)((char*)block + needed_size);
    new_block->size  = block->size - needed_size;
    new_block->magic = magic;
    new_block->chunk = block->chunk;
    new_block->phys_prev = block;
    new_block->phys_next = block->phys_next;
    if (new_block->phys_next) {
        new_block->phys_next->phys_prev = new_block;
    }
    block->phys_next = new_block;
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
    u64 f = irq_save();
    if (!is_init) {
        if (grow_heap(0)) {
            is_init = 1;
        }
    }
    irq_restore(f);
}

void* allocate(size_t size) {
    if (size == 0) return NULL_PTR;

    u64 f = irq_save();
    if (!is_init) {
        init_allocator();
        if (!is_init) { irq_restore(f); return NULL_PTR; }
    }

    const size_t needed_size = ALIGN(size) + sizeof(Block);

    for (;;) {
        Block* curr = free_list_head;
        while (curr) {
            if (curr->size >= needed_size) {
                remove_from_free_list(curr);
                curr->is_free = 0;
                curr->magic = magic;

                if (curr->size > needed_size + MIN_BLOCK_SIZE) {
                    split_block(curr, needed_size);
                }

                curr->chunk->used_blocks++;
                total_allocated += curr->size;
                irq_restore(f);
                return (void*)(curr + 1);
            }
            curr = curr->free_next;
        }

        if (!grow_heap(needed_size)) {
            err("Failed to allocate memory: out of heap space\n");
            irq_restore(f);
            return NULL_PTR;
        }
        /* fresh chunk is in the free list - rescan */
    }
}

void release(void* ptr) {
    if (!ptr) return;

    u64 f = irq_save();
    Block* block = (Block*)ptr - 1;

    if (block->magic != magic) {
        err("Segmentation fault (Invalid magic)\n");
        irq_restore(f);
        return;
    }
    if (block->is_free) {
        err("Double free detected!\n");
        irq_restore(f);
        return;
    }
    Chunk* c = block->chunk;
    if (!c || c->magic != CHUNK_MAGIC) {
        err("Heap corruption (bad chunk)\n");
        irq_restore(f);
        return;
    }

    total_allocated -= block->size;
    block->is_free = 1;

    block = merge_with_next(block);

    if (block->phys_prev && block->phys_prev->is_free) {
        Block* prev = block->phys_prev;
        remove_from_free_list(prev);
        prev->size += block->size;
        prev->phys_next = block->phys_next;
        if (block->phys_next) {
            block->phys_next->phys_prev = prev;
        }
        block = prev;
    }

    insert_into_free_list(block);

    if (--c->used_blocks == 0) {
        remove_from_free_list(block);
        free_chunk(c);
    }
    irq_restore(f);
}

size_t get_total_allocated_size(void) {
    return total_allocated;
}

size_t allocation_size(void* ptr) {
    if (!ptr) return 0;
    Block* b = (Block*)ptr - 1;
    if (b->magic != magic || b->is_free) return 0;
    return b->size - sizeof(Block);
}
