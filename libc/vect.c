#include "vect.h"

void resize(Vect* v);

int size(void** arr){
    return (int)sizeof(arr)/sizeof(*arr[0]);
}

void init_vect(Vect* v, int element_size){
    v->e_size = element_size;
    v->cappacity = 1;
    v->size = 0;
    v->arr = allocate(v->e_size);
}

void vect_add_elem(Vect* v, void* elem){
    if(v->size == v->cappacity){
        resize(v);
    }
    v->arr[v->size++] = elem;
}


void resize(Vect* v){
    /*copy*/
    void** old_arr = allocate(v->size * v->e_size);
    mem_cpy(old_arr, v->arr, v->size);
    
    release(v->arr);
    v->cappacity *= 2;
    v->arr = allocate(v->e_size * v->cappacity);
    mem_cpy(v->arr, old_arr, v->size);
    release(old_arr);
}

void delete_vect(Vect* v){
    release(v->arr);
    v->cappacity = 0;
    v->size = 0;
    v->e_size = 0;
}

void** get_vect(const Vect* v) {
    return v->arr;
}

int get_vect_size(const Vect* v){
    return v->size;
}