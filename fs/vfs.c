#include "vfs.h"
#include "../libc/string.h"
#include "../libc/memory.h"
#include "../libc/stdlib.h"

vfs_node_t *vfs_root = NULL;
vfs_node_t *vfs_cwd = NULL;

void vfs_init(void) {
    vfs_root = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    memset(vfs_root, 0, sizeof(vfs_node_t));
    strcpy(vfs_root->name, "/");
    vfs_root->flags = VFS_FLAG_DIRECTORY | VFS_FLAG_MOUNTPOINT;
    vfs_root->parent = vfs_root;
}

vfs_node_t *vfs_namei(const char *path) {
    if (!path || path[0] != '/') return NULL;

    vfs_node_t *current = vfs_root;
    char path_copy[512];
    strncpy(path_copy, path, sizeof(path_copy));
    
    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);

    while (token != NULL) {
        if (current->flags & VFS_FLAG_MOUNTPOINT && current->device) {
            current = (vfs_node_t *)current->device;
        }

        if (!current->ops || !current->ops->finddir) {
            return NULL;
        }

        vfs_node_t *next = current->ops->finddir(current, token);
        if (!next) {
            return NULL;
        }
        
        current = next;
        token = strtok_r(NULL, "/", &saveptr);
    }

    return current;
}

vfs_file_t *vfs_open(const char *path, uint32_t flags) {
    vfs_node_t *node = vfs_namei(path);
    
    if (!node && (flags & O_CREAT)) {
        char parent_path[256];
        char filename[256];
        
        const char *last_slash = strrchr(path, '/');
        if (!last_slash) {
            return NULL;
        }
        
        uint32_t parent_len = last_slash - path;
        if (parent_len == 0) parent_len = 1;
        strncpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
        
        strncpy(filename, last_slash + 1, sizeof(filename));
        
        vfs_node_t *parent = vfs_resolve_path(parent_path);
        if (!parent || !(parent->flags & VFS_FLAG_DIRECTORY)) {
            return NULL;
        }
        
        if (!parent->ops || !parent->ops->create) {
            return NULL;
        }
        
        node = parent->ops->create(parent, filename);
        if (!node) {
            return NULL;
        }
    }
    
    if (!node) return NULL;
    
    if ((flags & O_TRUNC) && node->ops && node->ops->truncate) {
        node->ops->truncate(node);
    }
    
    vfs_file_t *file = (vfs_file_t *)malloc(sizeof(vfs_file_t));
    file->node = node;
    file->offset = 0;
    file->flags = flags & 0xFF;
    return file;
}

int vfs_read(vfs_file_t *file, char *buffer, uint32_t size) {
    if (!file || !file->node || !file->node->ops || !file->node->ops->read) {
        return -1;
    }
    
    int bytes_read = file->node->ops->read(file->node, file->offset, size, buffer);
    if (bytes_read > 0) {
        file->offset += bytes_read;
    }
    return bytes_read;
}

int vfs_write(vfs_file_t *file, const char *buffer, uint32_t size) {
    if (!file || !file->node) {
        return -1;
    }
    
    if (file->flags == O_RDONLY) {
        return -1;
    }
    
    if (!file->node->ops || !file->node->ops->write) {
        return -1;
    }
    
    int bytes_written = file->node->ops->write(file->node, file->offset, size, buffer);
    
    if (bytes_written > 0) {
        file->offset += bytes_written;
        
        if (file->offset > file->node->size) {
            file->node->size = (uint32_t)file->offset;
        }
    }
    
    return bytes_written;
}

vfs_node_t *vfs_mkdir(const char *path) {
    if (!path || path[0] != '/') {
        return NULL;
    }
    
    const char *last_slash = strrchr(path, '/');
    if (!last_slash) {
        return NULL;
    }
    
    char parent_path[256];
    char dirname[256];
    
    uint32_t parent_len = last_slash - path;
    if (parent_len == 0) parent_len = 1;
    strncpy(parent_path, path, parent_len);
    parent_path[parent_len] = '\0';
    
    strncpy(dirname, last_slash + 1, sizeof(dirname));
    
    vfs_node_t *parent = vfs_resolve_path(parent_path);
    if (!parent || !(parent->flags & VFS_FLAG_DIRECTORY)) {
        return NULL;
    }
    
    if (!parent->ops || !parent->ops->mkdir) {
        return NULL;
    }
    
    return parent->ops->mkdir(parent, dirname);
}

int vfs_readdir(const char* path, uint32_t index, vfs_dirent_t *dirent)
{
    if (!path || !dirent)
        return -1;

    vfs_node_t *dir = vfs_resolve_path(path);
    if(!dir)
        return -4;

    if (!(dir->flags & VFS_FLAG_DIRECTORY)) {
        return -2;
    }
    
    if (!dir->ops || !dir->ops->readdir) {
        return -3;
    }
    
    return dir->ops->readdir(dir, index, dirent);
}

void vfs_close(vfs_file_t *file) {
    if (file) {
        free(file);
    }
}

vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path) return NULL;
    
    if (path[0] == '/') {
        return vfs_namei(path);
    }
    
    char full_path[512];
    
    if (vfs_cwd && vfs_cwd->name[0] != '\0') {
        strcpy(full_path, vfs_cwd->name);
        
        uint32_t len = strlen(full_path);
        if (len > 0 && full_path[len - 1] != '/') {
            append_str(full_path, "/");
        }
        
        append_str(full_path, path);
    } else {
        full_path[0] = '/';
        strcpy(full_path + 1, path);
    }
    
    return vfs_namei(full_path);
}

int vfs_change_cwd(const char *path)
{
    if (!path)
        return -1;

    vfs_node_t *dir = vfs_resolve_path(path);
    if (!dir)
        return -4;

    vfs_cwd = dir;
    return 0;
}