#include "pipe.h"
#include "../libc/stdlib.h"
#include "../libc/memory.h"
#include "../multitasking/mt.h"

#define PIPE_SIZE 4096

static int pipe_empty(pipe_t *p) { return p->head == p->tail; }
static int pipe_full(pipe_t *p)  { return p->head - p->tail == PIPE_SIZE; }

static int pipe_read(vfs_node_t *n, uint64_t off, uint32_t sz, void *b)
{
    (void)off;
    pipe_t *p = n->device;
    uint8_t *out = b;
    uint32_t done = 0;

    while (done < sz) {
        if (!pipe_empty(p)) {
            out[done++] = p->buf[p->tail++ % PIPE_SIZE];
        } else if (p->writers == 0) {
            break;
        } else {
            yield();
        }
    }
    return (int)done;               /* 0 = EOF */
}

static int pipe_read_close(vfs_node_t *n)
{
    pipe_t *p = n->device;
    p->readers--;
    if (p->readers == 0 && p->writers == 0)
        free(p);
    return 0;
}

static int pipe_write(vfs_node_t *n, uint64_t off, uint32_t sz, const void *b)
{
    (void)off;
    pipe_t *p = n->device;
    const uint8_t *in = b;
    uint32_t done = 0;

    while (done < sz) {
        if (p->readers == 0)
            return done ? (int)done : -1;   /* EPIPE */
        if (!pipe_full(p)) {
            p->buf[p->head++ % PIPE_SIZE] = in[done++];
        } else {
            yield();
        }
    }
    return (int)done;
}

static int pipe_write_close(vfs_node_t *n)
{
    pipe_t *p = n->device;
    p->writers--;
    if (p->readers == 0 && p->writers == 0)
        free(p);
    return 0;
}

static vfs_ops_t pipe_read_ops  = { .read = pipe_read,  .close = pipe_read_close };
static vfs_ops_t pipe_write_ops = { .write = pipe_write, .close = pipe_write_close };

static vfs_file_t *pipe_make_file(pipe_t *p, int is_write)
{
    vfs_node_t *n = malloc(sizeof(vfs_node_t));
    vfs_file_t *f = malloc(sizeof(vfs_file_t));
    if (!n || !f) { free(n); free(f); return NULL; }

    memset(n, 0, sizeof(*n));
    n->flags     = VFS_FLAG_DEVICE;
    n->device    = p;
    n->ops       = is_write ? &pipe_write_ops : &pipe_read_ops;
    n->ref_count = 1;

    f->node   = n;
    f->offset = 0;
    f->flags  = is_write ? O_WRONLY : O_RDONLY;

    if (is_write) p->writers++; else p->readers++;
    return f;
}

int pipe_create(vfs_file_t **rf, vfs_file_t **wf)
{
    pipe_t *p = malloc(sizeof(pipe_t));
    if (!p) return -1;
    memset(p, 0, sizeof(*p));

    *rf = pipe_make_file(p, 0);
    *wf = pipe_make_file(p, 1);
    if (!*rf || !*wf) {
        if (*rf) vfs_close(*rf);
        if (*wf) vfs_close(*wf);
        free(p);
        return -1;
    }
    return 0;
}