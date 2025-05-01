#include "stdio.h"

void print(void* msg){
    krnl_print((char*)msg);
}