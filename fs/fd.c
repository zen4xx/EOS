#include "fd.h"
#include "vfs.h"

static int last_free_fd = -1;  // fd_close must install it on own rn freed fd

static int get_free_fd(const fd_table_t* table)
{

    if (last_free_fd != -1) // attemping to get fd with O(1)
    {
        int tmp = last_free_fd;
        last_free_fd = -1;
        return tmp;
    }

    for (int i = 0; i < table->max_fd; ++i)
    {
        if (table->files[i].is_free == 1)
            return i;
    }
    return -1;
}

fd_table_t* fd_table_alloc(int max_fd)
{
    if (max_fd <= 0)
        return NULL;

    fd_table_t* res = malloc(sizeof(fd_table_t));
    fd_file* files = malloc(sizeof(fd_file) * max_fd);
    
    if (!files || !res)
        return NULL;

    res->max_fd = max_fd;
    
    for (int i = 0; i < max_fd; ++i)
    {
        files[i].file = NULL;
        files[i].is_free = 1;
    }

    res->files = files;
    
    return res;
}

int fd_open(fd_table_t* table, const char* path, uint32_t flags)
{
    if (!table || !path)
        return -1;

    int fd = get_free_fd(table);

    if (fd < 0)
        return -2;

    vfs_file_t *file = vfs_open(path, flags);

    if (!file)
        return -3;

    table->files[fd].file = file;
    table->files[fd].is_free = 0;

    return 0;
}

int fd_close(fd_table_t *table, int fd)
{
    if (!table)
        return -1;

    if (fd < 0 || fd > table->max_fd)
        return -2;

    if (table->files[fd].is_free == 0)
    {
        vfs_close(table->files[fd].file);
        table->files[fd].is_free = 1;
        last_free_fd = fd;
    }

    return 0;
}

int fd_read(fd_table_t *table, int fd, void *buffer, size_t size)
{
    if (!table)
        return -1;

    if (fd < 0 || fd > table->max_fd)
        return -2;

    if (table->files[fd].is_free == 1) 
        return -3;

    return vfs_read(table->files[fd].file, buffer, size);
}

int fd_write(fd_table_t *table, int fd, const void *buffer, size_t size)
{
    if (!table)
        return -1;

    if (fd < 0 || fd > table->max_fd)
        return -2;

    if (table->files[fd].is_free == 1) 
        return -3;

    return vfs_write(table->files[fd].file, buffer, size);
}