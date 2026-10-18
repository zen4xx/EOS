#include "vfs.h"
#include "../libc/string.h"
#include "../libc/memory.h"
#include "../libc/stdlib.h"

vfs_node_t *vfs_root = NULL;
vfs_node_t *vfs_cwd = NULL;

static char vfs_cwd_path[VFS_MAX_PATH] = "/";

static vfs_node_t *vfs_follow_mount(vfs_node_t *node)
{
    int limit = 16;

    while (node &&
           (node->flags & VFS_FLAG_MOUNTPOINT) &&
           node->device &&
           limit-- > 0) {
        vfs_node_t *next = (vfs_node_t *)node->device;

        if (next == node)
            break;

        vfs_node_hold(next);
        vfs_node_release(node);
        
        node = next;
    }

    return node;
}

static int vfs_normalize_path(const char *path,
                              const char *base,
                              char *out,
                              uint32_t out_size)
{
    if (!path || !out || out_size < 2)
        return -1;

    out[0] = '/';
    out[1] = '\0';

    uint32_t len = 1;

    if (path[0] != '/') {
        if (!base || base[0] != '/')
            base = "/";

        uint32_t blen = strlen(base);

        if (blen == 0 || blen >= out_size)
            return -1;

        memcpy(out, (void*)base, blen + 1);
        len = blen;

        if (len > 1 && out[len - 1] == '/') {
            out[len - 1] = '\0';
            len--;
        }
    }

    const char *p = path;

    while (*p == '/')
        p++;

    while (*p != '\0') {
        while (*p == '/')
            p++;

        if (*p == '\0')
            break;

        const char *start = p;

        while (*p != '\0' && *p != '/')
            p++;

        uint32_t comp_len = (uint32_t)(p - start);

        if (comp_len == 0 ||
            (comp_len == 1 && start[0] == '.')) {
            continue;
        }

        if (comp_len == 2 &&
            start[0] == '.' &&
            start[1] == '.') {
            if (len > 1) {
                uint32_t i = len;

                while (i > 1 && out[i - 1] != '/')
                    i--;

                if (i <= 1) {
                    len = 1;
                    out[1] = '\0';
                } else {
                    out[i - 1] = '\0';
                    len = i - 1;
                }
            }

            continue;
        }

        uint32_t need = len + (len > 1 ? 1 : 0) + comp_len;

        if (need >= out_size)
            return -1;

        if (len > 1)
            out[len++] = '/';

        memcpy(out + len, (void*)start, comp_len);
        len += comp_len;
        out[len] = '\0';
    }

    if (len == 0) {
        out[0] = '/';
        out[1] = '\0';
    }

    return 0;
}

static int vfs_split_path(const char *path,
                          char *parent,
                          uint32_t parent_size,
                          char *name,
                          uint32_t name_size)
{
    if (!path || path[0] != '/' || !parent || !name)
        return -1;

    uint32_t len = strlen(path);
    if (len <= 1)
        return -1;

    const char *name_start = path + len;

    while (name_start > path && *(name_start - 1) != '/')
        name_start--;

    uint32_t name_len = (uint32_t)(path + len - name_start);
    uint32_t parent_len = (uint32_t)(name_start - path);

    if (name_len == 0 || name_len >= name_size)
        return -1;

    if (parent_len > 1 && path[parent_len - 1] == '/')
        parent_len--;

    if (parent_len == 0)
        parent_len = 1;

    if (parent_len >= parent_size)
        return -1;

    memcpy(parent, (void*)path, parent_len);
    parent[parent_len] = '\0';

    memcpy(name, (void*)name_start, name_len);
    name[name_len] = '\0';

    return 0;
}

static vfs_node_t *vfs_namei_normalized(const char *path)
{
    if (!vfs_root || !path || path[0] != '/')
        return NULL;

    vfs_node_t *current = vfs_follow_mount(vfs_root);
    vfs_node_hold(current);

    if (!current)
        return NULL;

    const char *p = path;

    while (*p == '/')
        p++;

    while (*p != '\0') {
        while (*p == '/')
            p++;

        if (*p == '\0')
            break;

        const char *start = p;

        while (*p != '\0' && *p != '/')
            p++;

        uint32_t comp_len = (uint32_t)(p - start);

        if (comp_len == 0)
            continue;

        if (comp_len >= VFS_MAX_NAME)
            return NULL;

        char component[VFS_MAX_NAME];
        memcpy(component, (void*)start, comp_len);
        component[comp_len] = '\0';

        if (!(current->flags & VFS_FLAG_DIRECTORY))
            return NULL;

        if (!current->ops || !current->ops->finddir)
            return NULL;

        vfs_node_t *next = current->ops->finddir(current, component);

        vfs_node_release(current);

        if (!next)
            return NULL;

        current = vfs_follow_mount(next);

        if (!current)
            return NULL;
    }

    return current;
}

static void vfs_attach_node(vfs_node_t *node, vfs_node_t *parent, const char *name)
{
    if (!node) return;

    if (name && node->name[0] == '\0') {
        uint32_t i = 0;
        while (i < VFS_MAX_NAME - 1 && name[i]) {
            node->name[i] = name[i];
            i++;
        }
        node->name[i] = '\0';
    }
}

void vfs_node_hold(vfs_node_t *node)
{
    if (node) {
        node->ref_count++;
    }
}

void vfs_node_release(vfs_node_t *node)
{
    if (!node) return;
    
    if (node->ref_count == 0) {
        return; 
    }
    
    node->ref_count--;
    
    if (node->ref_count == 0 && node != vfs_root) {
        vfs_node_t *parent = node->parent;
        
        node->ops = NULL;
        node->parent = NULL;
        
        free(node);
        
        if (parent && parent != node) {
            vfs_node_release(parent);
        }
    }
}

void vfs_init(void)
{
    if (vfs_root) return;

    vfs_root = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    if (!vfs_root) return;

    memset(vfs_root, 0, sizeof(vfs_node_t));
    
    vfs_root->name[0] = '/';
    vfs_root->name[1] = '\0';
    vfs_root->flags = VFS_FLAG_DIRECTORY | VFS_FLAG_MOUNTPOINT;
    vfs_root->parent = vfs_root;
    vfs_root->ref_count = 1;

    vfs_cwd = vfs_root;
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
}

int vfs_mount(vfs_node_t *mountpoint, vfs_node_t *mount_root)
{
    if (!mountpoint || !mount_root)
        return -1;

    if (mountpoint == mount_root)
        return -1;

    if (!(mountpoint->flags & VFS_FLAG_DIRECTORY))
        return -1;

    if (!(mount_root->flags & VFS_FLAG_DIRECTORY))
        return -1;

    if ((mountpoint->flags & VFS_FLAG_MOUNTPOINT) && mountpoint->device)
        return -1;

    mountpoint->flags |= VFS_FLAG_MOUNTPOINT;
    mountpoint->device = mount_root;

    return 0;
}

vfs_node_t *vfs_namei(const char *path)
{
    if (!path)
        return NULL;

    char normalized[VFS_MAX_PATH];

    const char *base = (path[0] == '/') ? NULL : vfs_cwd_path;

    if (vfs_normalize_path(path, base, normalized, sizeof(normalized)) != 0)
        return NULL;

    return vfs_namei_normalized(normalized);
}

vfs_node_t *vfs_resolve_path(const char *path)
{
    return vfs_namei(path);
}

vfs_file_t *vfs_open(const char *path, uint32_t flags)
{
    if (!path)
        return NULL;

    char normalized[VFS_MAX_PATH];

    if (vfs_normalize_path(path, vfs_cwd_path, normalized, sizeof(normalized)) != 0)
        return NULL;

    uint32_t access = flags & O_ACCMODE;

    if (access > O_RDWR)
        return NULL;

    vfs_node_t *node = vfs_namei_normalized(normalized);

    if (node && (flags & O_CREAT) && (flags & O_EXCL)) {
        vfs_node_release(node);
        return NULL;
    }

    if (!node && (flags & O_CREAT)) {
        char parent_path[VFS_MAX_PATH];
        char filename[VFS_MAX_NAME];

        if (vfs_split_path(normalized,
                           parent_path,
                           sizeof(parent_path),
                           filename,
                           sizeof(filename)) != 0) {
            return NULL;
        }

        vfs_node_t *parent = vfs_namei_normalized(parent_path);

        if (!parent || !(parent->flags & VFS_FLAG_DIRECTORY))
            return NULL;

        if (!parent->ops || !parent->ops->create)
            return NULL;

        node = parent->ops->create(parent, filename);

        if (!node)
            return NULL;

        node->size = 0;

        if (!(node->flags & (VFS_FLAG_FILE |
                             VFS_FLAG_DIRECTORY |
                             VFS_FLAG_DEVICE |
                             VFS_FLAG_MOUNTPOINT))) {
            node->flags |= VFS_FLAG_FILE;
        }

        vfs_attach_node(node, parent, filename);

        vfs_node_release(parent);
    }

    if (!node)
        return NULL;

    if (flags & O_DIRECTORY) {
        if (!(node->flags & VFS_FLAG_DIRECTORY))
            return NULL;
    }

    if (access != O_RDONLY &&
        (node->flags & VFS_FLAG_DIRECTORY)) {
        return NULL;
    }

    if ((access == O_RDONLY || access == O_RDWR) &&
        !(node->flags & VFS_FLAG_DIRECTORY)) {
        if (!node->ops || !node->ops->read)
            return NULL;
    }

    if (access == O_WRONLY || access == O_RDWR) {
        if (!node->ops || !node->ops->write)
            return NULL;
    }

    if ((flags & O_TRUNC) &&
        access != O_RDONLY &&
        !(node->flags & VFS_FLAG_DIRECTORY)) {
        if (node->ops && node->ops->truncate) {
            if (node->ops->truncate(node) < 0)
                return NULL;
        } else if (node->size != 0) {
            return NULL;
        }
    }

    vfs_file_t *file = (vfs_file_t *)malloc(sizeof(vfs_file_t));

    if (!file)
    {
        vfs_node_release(node);
        return NULL;
    }
    file->node = node;
    file->flags = flags & (O_ACCMODE | O_APPEND);
    file->offset = (flags & O_APPEND) ? node->size : 0;

    return file;
}

int vfs_read(vfs_file_t *file, char *buffer, uint32_t size)
{
    if (!file || !file->node || !buffer)
        return -1;

    if (size == 0)
        return 0;

    if ((file->flags & O_ACCMODE) == O_WRONLY)
        return -1;

    if (file->node->flags & VFS_FLAG_DIRECTORY)
        return -1;

    if (!file->node->ops || !file->node->ops->read)
        return -1;

    if ((file->node->flags & VFS_FLAG_FILE) &&
        file->offset >= file->node->size) {
        return 0;
    }

    int bytes_read = file->node->ops->read(file->node,
                                           file->offset,
                                           size,
                                           buffer);

    if (bytes_read > 0)
        file->offset += (uint64_t)bytes_read;

    return bytes_read;
}

int vfs_write(vfs_file_t *file, const char *buffer, uint32_t size)
{
    if (!file || !file->node || !buffer)
        return -1;

    if (size == 0)
        return 0;

    if ((file->flags & O_ACCMODE) == O_RDONLY)
        return -1;

    if (file->node->flags & VFS_FLAG_DIRECTORY)
        return -1;

    if (!file->node->ops || !file->node->ops->write)
        return -1;

    if (file->flags & O_APPEND)
        file->offset = file->node->size;

    int bytes_written = file->node->ops->write(file->node,
                                               file->offset,
                                               size,
                                               buffer);

    if (bytes_written > 0) {
        file->offset += (uint64_t)bytes_written;

        if (file->offset > file->node->size)
            file->node->size = file->offset;
    }

    return bytes_written;
}

vfs_node_t *vfs_mkdir(const char *path)
{
    if (!path)
        return NULL;

    char normalized[VFS_MAX_PATH];

    if (vfs_normalize_path(path, vfs_cwd_path, normalized, sizeof(normalized)) != 0)
        return NULL;

    vfs_node_t *existing = vfs_namei_normalized(normalized);

    if (existing)
    {
        vfs_node_release(existing);
        return NULL;
    }

    char parent_path[VFS_MAX_PATH];
    char dirname[VFS_MAX_NAME];

    if (vfs_split_path(normalized,
                       parent_path,
                       sizeof(parent_path),
                       dirname,
                       sizeof(dirname)) != 0) {
        return NULL;
    }

    vfs_node_t *parent = vfs_namei_normalized(parent_path);

    if (!parent || !(parent->flags & VFS_FLAG_DIRECTORY))
        return NULL;

    if (!parent->ops || !parent->ops->mkdir)
        return NULL;

    vfs_node_t *dir = parent->ops->mkdir(parent, dirname);

    if (dir) {
        dir->flags |= VFS_FLAG_DIRECTORY;
        vfs_attach_node(dir, parent, dirname);
    }

    dir->ops->sync(dir);

    vfs_node_release(parent);
    return dir;
}

int vfs_readdir(const char *path, uint32_t index, vfs_dirent_t *dirent)
{
    if (!path || !dirent)
        return -1;

    char normalized[VFS_MAX_PATH];

    if (vfs_normalize_path(path, vfs_cwd_path, normalized, sizeof(normalized)) != 0)
        return -1;

    vfs_node_t *dir = vfs_namei_normalized(normalized);

    if (!dir)
        return -4;

    if (!(dir->flags & VFS_FLAG_DIRECTORY))
        return -2;

    if (!dir->ops || !dir->ops->readdir)
        return -3;

    return dir->ops->readdir(dir, index, dirent);
}

int vfs_change_cwd(const char *path)
{
    if (!path) return -1;

    char normalized[VFS_MAX_PATH];
    if (vfs_normalize_path(path, vfs_cwd_path, normalized, sizeof(normalized)) != 0) {
        return -1;
    }

    vfs_node_t *dir = vfs_namei_normalized(normalized);
    if (!dir) return -4;

    if (!(dir->flags & VFS_FLAG_DIRECTORY)) {
        vfs_node_release(dir);
        return -2;
    }

    uint32_t len = strlen(normalized);
    if (len >= sizeof(vfs_cwd_path)) {
        vfs_node_release(dir);
        return -1;
    }

    memcpy(vfs_cwd_path, normalized, len + 1);
    
    vfs_node_release(vfs_cwd);
    vfs_cwd = dir;

    return 0;
}

void vfs_close(vfs_file_t *file)
{
    if (!file) return;
    
    if (file->node) {
        if (file->node->ops && file->node->ops->sync) {
            file->node->ops->sync(file->node);
        }
        vfs_node_release(file->node);
        file->node = NULL;
    }

    free(file);
}