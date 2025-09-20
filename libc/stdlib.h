#include "../cpu/types.h"

void* malloc(u32 size);
void free(void* ptr);
void* realloc(void* ptr, u32 size);
