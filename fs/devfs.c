#include "devfs.h"
#include "../libc/string.h"
#include "../libc/stdlib.h"
#include "../libc/memory.h"
#include "../drivers/screen.h"
#include "../drivers/ahci.h"
#include "../kernel/kernel.h"


typedef struct devfs_entry {
    vfs_node_t *node;
    struct devfs_entry *next;
} devfs_entry_t;

static vfs_node_t   *devfs_root  = NULL;
static devfs_entry_t *devfs_list = NULL;
static uint32_t      devfs_next_inode = 2;

static vfs_node_t *devfs_finddir(vfs_node_t *dir, const char *name)
{
    (void)dir;
    for (devfs_entry_t *e = devfs_list; e; e = e->next) {
        if (strcmp(e->node->name, name) == 0) {
            vfs_node_hold(e->node);
            return e->node;
        }
    }
    return NULL;
}

static int devfs_readdir(vfs_node_t *dir, uint32_t index, vfs_dirent_t *dirent)
{
    (void)dir;
    devfs_entry_t *e = devfs_list;
    while (e && index--)
        e = e->next;
    if (!e)
        return -1;

    strncpy(dirent->name, e->node->name, VFS_MAX_NAME - 1);
    dirent->name[VFS_MAX_NAME - 1] = '\0';
    dirent->inode = e->node->inode;
    dirent->flags = e->node->flags;   /* VFS_FLAG_DEVICE */
    return 0;
}

static vfs_ops_t devfs_root_ops = {
    .finddir = devfs_finddir,
    .readdir = devfs_readdir,
};

vfs_node_t *devfs_register(const char *name, vfs_ops_t *ops, void *priv)
{
    if (!devfs_root || !name || !ops || devfs_finddir(devfs_root, name))
        return NULL;

    vfs_node_t *node = malloc(sizeof(vfs_node_t));
    devfs_entry_t *e = malloc(sizeof(devfs_entry_t));
    if (!node || !e) { free(node); free(e); return NULL; }

    memset(node, 0, sizeof(*node));
    strncpy(node->name, name, VFS_MAX_NAME - 1);
    node->flags     = VFS_FLAG_DEVICE;
    node->inode     = devfs_next_inode++;
    node->ops       = ops;
    node->device    = priv;
    node->parent    = devfs_root;
    node->ref_count = 1;
    vfs_node_hold(devfs_root);

    e->node = node;
    e->next = devfs_list;
    devfs_list = e;
    return node;
}

static int devfs_nop_truncate(vfs_node_t *n) { (void)n; return 0; }

/* /dev/null */
static int null_read (vfs_node_t *n, uint64_t off, uint32_t sz, void *b)
{ (void)n; (void)off; (void)sz; (void)b; return 0; }        /* EOF */
static int null_write(vfs_node_t *n, uint64_t off, uint32_t sz, const void *b)
{ (void)n; (void)off; (void)b; return (int)sz; }

static vfs_ops_t null_ops = {
    .read = null_read, .write = null_write, .truncate = devfs_nop_truncate,
};

/* /dev/zero */
static int zero_read(vfs_node_t *n, uint64_t off, uint32_t sz, void *b)
{ (void)n; (void)off; memset(b, 0, sz); return (int)sz; }

static vfs_ops_t zero_ops = {
    .read = zero_read, .write = null_write, .truncate = devfs_nop_truncate,
};

static int console_write(vfs_node_t *n, uint64_t off, uint32_t sz, const void *b)
{
    (void)n; (void)off;
    const char *s = b;
    for (uint32_t i = 0; i < sz; i++)
        print_char(s[i], -1, -1, 0x0f);
    return (int)sz;
}
static int console_read(vfs_node_t *n, uint64_t off, uint32_t sz, void *b)
{ (void)n; (void)off; (void)sz; (void)b; return 0; }

static vfs_ops_t console_ops = {
    .read = console_read, .write = console_write, .truncate = devfs_nop_truncate,
};

static int kbd_read(vfs_node_t *n, uint64_t off, uint32_t sz, void *b)
{
    (void)n; (void)off;
    if (sz == 0) return 0;
    _current_char = '\0';
    asm volatile("sti");
    while (_current_char == '\0')
        asm volatile("hlt");
    asm volatile("cli");
    ((char *)b)[0] = _current_char;
    return 1;
}
static vfs_ops_t kbd_ops = { .read = kbd_read };

static int sda_read(vfs_node_t *n, uint64_t off, uint32_t sz, void *b)
{
    (void)n;
    uint8_t *out = b, sec[512];
    uint32_t done = 0;
    while (done < sz) {
        uint64_t pos    = off + done;
        uint32_t within = (uint32_t)(pos % 512);
        uint32_t chunk  = 512 - within;
        if (chunk > sz - done) chunk = sz - done;
        if (ahci_read_sector(pos / 512, sec, 1) != 0)
            return done ? (int)done : -1;
        memcpy(out + done, sec + within, chunk);
        done += chunk;
    }
    return (int)done;
}

static int sda_write(vfs_node_t *n, uint64_t off, uint32_t sz, const void *b)
{
    (void)n;
    const uint8_t *in = b;
    uint8_t sec[512];
    uint32_t done = 0;
    while (done < sz) {
        uint64_t pos    = off + done;
        uint64_t lba    = pos / 512;
        uint32_t within = (uint32_t)(pos % 512);
        uint32_t chunk  = 512 - within;
        if (chunk > sz - done) chunk = sz - done;
        if (ahci_read_sector(lba, sec, 1) != 0)
            return done ? (int)done : -1;
        memcpy(sec + within, in + done, chunk);
        if (ahci_write_sector(lba, sec, 1) != 0)
            return done ? (int)done : -1;
        done += chunk;
    }
    return (int)done;
}

static vfs_ops_t sda_ops = {
    .read = sda_read, .write = sda_write, .truncate = devfs_nop_truncate,
};

int devfs_init(void)
{
    if (devfs_root)
        return 0;

    devfs_root = malloc(sizeof(vfs_node_t));
    if (!devfs_root)
        return -1;
    memset(devfs_root, 0, sizeof(vfs_node_t));
    strncpy(devfs_root->name, "dev", VFS_MAX_NAME - 1);
    devfs_root->flags     = VFS_FLAG_DIRECTORY;
    devfs_root->inode     = 1;
    devfs_root->parent    = devfs_root;
    devfs_root->ops       = &devfs_root_ops;
    devfs_root->ref_count = 1;

    devfs_register("null",    &null_ops,    NULL);
    devfs_register("zero",    &zero_ops,    NULL);
    devfs_register("console", &console_ops, NULL);
    devfs_register("kbd",     &kbd_ops,     NULL);
    devfs_register("sda",     &sda_ops,     NULL);

    return vfs_mount_path("/dev", devfs_root);
}