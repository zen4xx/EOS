#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

void print(const char* msg) {
    write(1, msg, strlen(msg)); // write to stdout
}

char getc(void) {
    char buf;
    read(0, &buf, 1);
    return buf;
}