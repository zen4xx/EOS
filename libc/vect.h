#ifndef VECT_H
#define VECT_H

#include "stdint.h"
#include "memory.h"

typedef struct {
    void *arr;
    size_t chunk_size;
    size_t size;
    size_t capacity;
}vect_t;

void vect_init(vect_t *vect, size_t chunk_size);
void vect_push(vect_t *vect, void* pElem);
void vect_delete(vect_t *vect);
void vect_clear(vect_t *vect);

static inline void* vect_get(const vect_t *vect, size_t index)
{
    return (void*)((char*)vect->arr+(vect->chunk_size * index)); // cast to char* to address ariphmetic
}

static inline size_t vect_get_size(const vect_t *vect)
{
    return vect->size;
}

#endif
