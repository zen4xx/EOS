#ifndef STDLIB_H
#define STDLIB_H

#include "stddef.h"
#include "stdint.h"

#define VFS_MAX_NAME 256

typedef struct 
{
    char name[VFS_MAX_NAME];
    uint32_t inode;
    uint32_t flags;
}dirent_t;

void* malloc(uint64_t size);
void free(void* ptr);
void* realloc(void* ptr, uint64_t size);
uint64_t malloc_info(void); // return a total allocated size in bytes

int fopen(const char *path, uint32_t flags);
int fclose(int fd);
int fread(int fd, void *buffer, size_t size);
int fwrite(int fd, const void *buffer, size_t size);
int mkdir(const char *path);
int readdir(const char *path, uint32_t index, dirent_t *entry);
int get_cwd(char *buffer, uint32_t size);
int change_cwd(const char *path);

#endif
