#ifndef VECT_H
#define VECT_H
#include "alloc.h"
#include "memory.h"
#include "libc.h"

typedef struct{
    int cappacity;
    int size;
    int e_size;
    void** arr;
}Vect;

void init_vect(Vect* v);
void vect_add_elem(Vect* v, void* elem);
void delete_vect(Vect* v);
void** get_vect(const Vect* v);
int get_vect_size(const Vect* v);
void clear_vect(Vect* v);

#endif