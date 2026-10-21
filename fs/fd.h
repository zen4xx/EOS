#ifndef FD_H
#define FD_H

#include "vfs.h"
#include "../libc/stdint.h"
#include "../libc/stdlib.h"

typedef struct 
{
    vfs_file_t* file;
    char is_free;
} fd_file;

typedef struct 
{
    fd_file *files;
    int max_fd;
} fd_table_t;

fd_table_t* fd_table_alloc(int max_fd);
static inline void fd_table_free(fd_table_t* table) { free(table->files); free(table); };

int fd_open(fd_table_t* table, const char* path, uint32_t flags);    // returns int fd decriptor and negative number if failed
int fd_close(fd_table_t* table, int fd);                             // return 0 if success and negative number if failed

int fd_read(fd_table_t* table, int fd, void* buffer, size_t size);
int fd_write(fd_table_t* table, int fd, const void* buffer, size_t size);

#endif