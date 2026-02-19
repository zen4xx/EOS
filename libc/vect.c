#include "vect.h"
#include "stdlib.h"

static void resize(vect_t *vect)
{
    size_t new_cap = vect->capacity * 2;
    void* new_arr = (void*)malloc(new_cap * vect->chunk_size);
    memcpy(new_arr, vect->arr, vect->size * vect->chunk_size);
    free(vect->arr);
    vect->arr = new_arr;
    vect->capacity = new_cap;
}

void vect_init(vect_t *vect, size_t chunk_size)
{
    vect->chunk_size = chunk_size;
    vect->size = 0;
    vect->capacity = 2;
    vect->arr = malloc(vect->capacity * chunk_size);
}

void vect_push(vect_t *vect, void *pElem)
{
    if (vect->size >= vect->capacity)
        resize(vect);

    memcpy((char*)vect->arr + (vect->size * vect->chunk_size), pElem, vect->chunk_size);
    ++vect->size;
}

void vect_delete(vect_t *vect)
{
    free(vect->arr);
    vect->arr = NULL;
    vect->capacity = 0;
    vect->size = 0;
}

void vect_clear(vect_t *vect)
{
    free(vect->arr);
    vect->size = 0;
    vect->capacity = 2;
    vect->arr = malloc(vect->capacity * vect->chunk_size);
}
