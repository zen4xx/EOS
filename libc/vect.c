#include "vect.h"

void resize(Vect* v);

void init_vect(Vect* v, int element_size){
    v->e_size = sizeof(void*);
    v->cappacity = 2;
    v->size = 0;
    v->arr = allocate(v->cappacity * v->e_size);
}

void vect_add_elem(Vect* v, void* elem){
    if(v->size == v->cappacity){
        resize(v);
    }
    v->arr[v->size++] = elem;
}


void resize(Vect* v){
    int new_cappacity = v->cappacity * 2;
    void** new_arr = (void**)allocate(new_cappacity * sizeof(void*));
    mem_cpy(new_arr, v->arr, v->size * sizeof(void*)); 
    release(v->arr);
    v->arr = new_arr;
    v->cappacity = new_cappacity;
}

void delete_vect(Vect* v){
    for (int i = 0; i < v->size; ++i) {
        if (v->arr[i]) release(v->arr[i]);
    }
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

void clear_vect(Vect* v){
    for (int i = 0; i < v->size; ++i) {
        if (v->arr[i]) release(v->arr[i]);
        v->arr[i] = 0;
    }
    v->size = 0;
}