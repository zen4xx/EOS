#ifndef DEVFS_H
#define DEVFS_H

#include "vfs.h"

int devfs_init(void);
vfs_node_t *devfs_register(const char *name, vfs_ops_t *ops, void *priv);

#endif