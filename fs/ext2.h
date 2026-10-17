#ifndef EXT2_H
#define EXT2_H

#include "../libc/stdint.h"
#include "../libc/string.h"
#include "vfs.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint16_t s_reserved_gdt_blocks;
    uint8_t  s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t  s_def_hash_version;
    uint8_t  s_jnl_backup_type;
    uint16_t s_reserved_word_pad;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_mkfs_time;
    uint32_t s_jnl_blocks[17];
    uint32_t s_reserved[167];
} ext2_superblock_t;

typedef struct {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
} ext2_group_desc_t;

typedef struct {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
} ext2_inode_t;

typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[];
} ext2_dir_entry_t;

#pragma pack(pop)

#define EXT2_MAGIC 0xEF53
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFREG 0x8000

typedef struct {
    ext2_superblock_t sb;

    uint32_t block_size;
    uint32_t first_data;
    uint32_t gdt_block;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t inode_size;
    uint32_t groups;

    uint32_t hint_block;
    uint32_t hint_inode;
    int      sb_dirty;
    int      readonly;
} ext2_fs_t;

extern vfs_node_t* vfs_root; // vfs.c

int ext2_mount(ext2_fs_t *fs);
int ext2_sync(ext2_fs_t *fs);
int ext2_read_file(ext2_fs_t *fs, uint32_t ino, uint64_t offset, void *dst, uint32_t len);
int ext2_write_file(ext2_fs_t *fs, uint32_t ino, uint64_t offset, const void *src, uint32_t len);
int ext2_create_file(ext2_fs_t *fs, uint32_t dir_ino, const char *name, uint32_t *new_inode);
int ext2_unlink(ext2_fs_t *fs, uint32_t dir_ino, const char *name);
int ext2_find_file(ext2_fs_t *fs, uint32_t dir_ino, const char *name, uint32_t *found_inode);
int ext2_inode_read(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *out);
int ext2_inode_write(ext2_fs_t *fs, uint32_t ino, const ext2_inode_t *in);

vfs_node_t *ext2_vfs_finddir(vfs_node_t *node, const char *name);
int ext2_vfs_read(vfs_node_t *node, uint64_t offset, uint32_t size, char *buffer);
int ext2_vfs_write(vfs_node_t *node, uint64_t offset, uint32_t size, const char* buffer);
vfs_node_t *ext2_vfs_create(vfs_node_t *parent, const char *name); 
int vfs_mount_ext2_root(void);

#endif