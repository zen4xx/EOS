#include "vect.h"
#include "memory.h"
#include "stdlib.h"

void resize(Vect* v);

void init_vect(Vect* v){
    v->e_size = sizeof(void*);
    v->capacity = 2;
    v->size = 0;
    v->arr = malloc(v->capacity * v->e_size);
}

void vect_add_elem(Vect* v, void* elem){
    if(v->size >= v->capacity){
        resize(v);
    }
    v->arr[v->size++] = elem;
}


void resize(Vect* v){
    int new_cappacity = v->capacity * 2;
    void** new_arr = (void**)malloc(new_cappacity * sizeof(void*));
    mem_cpy(new_arr, v->arr, v->size * sizeof(void*)); 
    free(v->arr);
    v->arr = new_arr;
    v->capacity = new_cappacity;
}

void delete_vect(Vect* v){
    if(v->arr)
        free(v->arr);
    v->capacity = 0;
    v->size = 0;
    v->e_size = 0;
}

void** get_vect(const Vect* v) {
    return v->arr;
}

int get_vect_size(const Vect* v){
    return v->size;
}

void clear_vect(Vect* v){
    if(v->arr)
        free(v->arr);
    v->capacity = 2;
    v->size = 0;
    v->arr = malloc(v->capacity * v->e_size);
}
