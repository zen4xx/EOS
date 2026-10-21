#ifndef VFS_H
#define VFS_H

#include "../libc/stdint.h"

#define VFS_MAX_PATH 512
#define VFS_MAX_NAME 256

#define VFS_FLAG_FILE       0x01
#define VFS_FLAG_DIRECTORY  0x02
#define VFS_FLAG_MOUNTPOINT 0x04
#define VFS_FLAG_DEVICE     0x08

#define O_RDONLY     0
#define O_WRONLY     1
#define O_RDWR       2
#define O_ACCMODE    0x3

#define O_CREAT      0x0100
#define O_TRUNC      0x0200
#define O_APPEND     0x0400
#define O_EXCL       0x0800
#define O_DIRECTORY  0x1000

typedef struct {
    char name[VFS_MAX_NAME];
    uint32_t inode;
    uint32_t flags;
} vfs_dirent_t;

typedef struct vfs_node vfs_node_t;

typedef struct vfs_ops {
    int (*read)(vfs_node_t *node, uint64_t offset, uint32_t size, void *buffer);
    int (*write)(vfs_node_t *node, uint64_t offset, uint32_t size, const void *buffer);
    int (*readdir)(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent);
    int (*truncate)(vfs_node_t *node);
    int (*sync)(vfs_node_t *node);
    vfs_node_t *(*finddir)(vfs_node_t *node, const char *name);
    vfs_node_t *(*mkdir)(vfs_node_t *parent, const char *name);
    vfs_node_t *(*create)(vfs_node_t *parent, const char *name);
} vfs_ops_t;

struct vfs_node {
    char name[VFS_MAX_NAME];
    uint32_t flags;
    uint64_t size;
    uint32_t inode;

    vfs_node_t *parent;
    void *device;
    vfs_ops_t *ops;
    uint32_t ref_count;
};

typedef struct vfs_file {
    vfs_node_t *node;
    uint64_t offset;
    uint32_t flags;
} vfs_file_t;

void vfs_init(void);
int vfs_mount(vfs_node_t *mountpoint, vfs_node_t *mount_root);
vfs_node_t *vfs_namei(const char *path);
vfs_node_t *vfs_resolve_path(const char *path);
vfs_file_t *vfs_open(const char *path, uint32_t flags);
int vfs_mkdir(const char *path);
int vfs_readdir(const char *path, uint32_t index, vfs_dirent_t *dirent);
int vfs_change_cwd(const char *path);
int vfs_get_cwd(char *dst, uint32_t size);
int vfs_read(vfs_file_t *file, void *buffer, uint32_t size);
int vfs_write(vfs_file_t *file, const void *buffer, uint32_t size);
void vfs_close(vfs_file_t *file);

void vfs_node_hold(vfs_node_t *node);
void vfs_node_release(vfs_node_t *node);

#endif