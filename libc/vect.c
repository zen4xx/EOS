#include "vect.h"

int size(void** arr){
    return (int)sizeof(*arr)/sizeof(*arr[0]);
}