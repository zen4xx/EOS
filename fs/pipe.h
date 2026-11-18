#ifndef PIPE_H
#define PIPE_H
#include "vfs.h"

typedef struct {
    uint8_t  buf[4096];
    uint32_t head, tail;
    uint32_t readers, writers;
} pipe_t;

int pipe_create(vfs_file_t **read_file, vfs_file_t **write_file);

#endif