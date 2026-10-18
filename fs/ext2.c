#include "ext2.h"
#include "../drivers/ahci.h"
#include "../libc/memory.h"
#include "../libc/stdlib.h"
#include "buf.h"
#include "vfs.h"

#define EXT2_SUPERBLOCK_OFFSET_LBA 2

static buf_t *group_desc(ext2_fs_t *fs, uint32_t g, ext2_group_desc_t **out)
{
    if (g >= fs->groups) return 0;

    uint32_t off  = g * sizeof(ext2_group_desc_t);
    uint32_t blk  = fs->gdt_block + off / fs->block_size;
    uint32_t within = off % fs->block_size;

    buf_t *b = bread(blk, fs->block_size);
    if (!b) return 0;
    *out = (ext2_group_desc_t *)(b->data + within);
    return b;
}

static inline int bit_get(const uint8_t *bm, uint32_t i) {
    return (bm[i >> 3] >> (i & 7)) & 1;
}
static inline void bit_set(uint8_t *bm, uint32_t i, int v) {
    if (v) bm[i >> 3] |=  (uint8_t)(1u << (i & 7));
    else   bm[i >> 3] &= (uint8_t)~(1u << (i & 7));
}

uint32_t ext2_alloc_block(ext2_fs_t *fs)
{
    if (fs->readonly) return 0;

    for (uint32_t n = 0; n < fs->groups; n++) {
        uint32_t g = (fs->hint_block + n) % fs->groups;

        ext2_group_desc_t *gd;
        buf_t *gb = group_desc(fs, g, &gd);
        if (!gb) return 0;

        if (!gd->bg_free_blocks_count) { brelse(gb); continue; }

        buf_t *bb = bread(gd->bg_block_bitmap, fs->block_size);
        if (!bb) { brelse(gb); return 0; }

        uint32_t base  = g * fs->blocks_per_group;
        uint32_t avail = fs->sb.s_blocks_count - fs->first_data - base;
        if (avail > fs->blocks_per_group) avail = fs->blocks_per_group;

        uint32_t found = 0xFFFFFFFF;
        for (uint32_t i = 0; i < avail; i++) {
            if (!bit_get(bb->data, i)) { found = i; break; }
        }

        if (found == 0xFFFFFFFF) { brelse(bb); brelse(gb); continue; }

        bit_set(bb->data, found, 1);
        bdirty(bb);

        gd->bg_free_blocks_count--;
        bdirty(gb);

        fs->sb.s_free_blocks_count--;
        fs->sb_dirty = 1;
        fs->hint_block = g;

        brelse(bb);
        brelse(gb);

        return fs->first_data + base + found;
    }
    return 0;
}

void ext2_free_block(ext2_fs_t *fs, uint32_t block)
{
    if (block <= fs->first_data) return;

    uint32_t rel = block - fs->first_data;
    uint32_t g   = rel / fs->blocks_per_group;
    uint32_t bit = rel % fs->blocks_per_group;

    ext2_group_desc_t *gd;
    buf_t *gb = group_desc(fs, g, &gd);
    if (!gb) return;

    buf_t *bb = bread(gd->bg_block_bitmap, fs->block_size);
    if (!bb) { brelse(gb); return; }

    if (!bit_get(bb->data, bit)) {
        brelse(bb); brelse(gb);
        return;
    }

    bit_set(bb->data, bit, 0);
    bdirty(bb);
    gd->bg_free_blocks_count++;
    bdirty(gb);
    fs->sb.s_free_blocks_count++;
    fs->sb_dirty = 1;

    brelse(bb);
    brelse(gb);
}

void ext2_free_inode(ext2_fs_t *fs, uint32_t ino)
{
    if (ino == 0 || ino > fs->sb.s_inodes_count) return;

    uint32_t g   = (ino - 1) / fs->inodes_per_group;
    uint32_t bit = (ino - 1) % fs->inodes_per_group;

    ext2_group_desc_t *gd;
    buf_t *gb = group_desc(fs, g, &gd);
    if (!gb) return;

    buf_t *ib = bread(gd->bg_inode_bitmap, fs->block_size);
    if (!ib) { brelse(gb); return; }

    if (!bit_get(ib->data, bit)) {
        brelse(ib); brelse(gb);
        return;
    }

    bit_set(ib->data, bit, 0);
    bdirty(ib);
    gd->bg_free_inodes_count++;
    bdirty(gb);
    fs->sb.s_free_inodes_count++;
    fs->sb_dirty = 1;

    brelse(ib);
    brelse(gb);
}

static int map_indirect(ext2_fs_t *fs, uint32_t table_blk,
                        uint32_t idx, int create, uint32_t *out)
{
    buf_t *b = bread(table_blk, fs->block_size);
    if (!b) return -1;

    uint32_t *arr = (uint32_t *)b->data;

    if (arr[idx]) {
        *out = arr[idx];
        brelse(b);
        return 0;
    }
    if (!create) {
        *out = 0;
        brelse(b);
        return 0;
    }

    uint32_t nb = ext2_alloc_block(fs);
    if (!nb) { brelse(b); return -1; }

    buf_t *zb = bnew(nb, fs->block_size);
    if (!zb) { ext2_free_block(fs, nb); brelse(b); return -1; }
    brelse(zb);

    arr[idx] = nb;
    bdirty(b);
    brelse(b);

    *out = nb;
    return 0;
}

static int map_slot(ext2_fs_t *fs, ext2_inode_t *in, int slot,
                    uint32_t idx, int create, uint32_t *out)
{
    if (!in->i_block[slot]) {
        if (!create) { *out = 0; return 0; }

        uint32_t nb = ext2_alloc_block(fs);
        if (!nb) return -1;

        buf_t *zb = bnew(nb, fs->block_size);
        if (!zb) { ext2_free_block(fs, nb); return -1; }
        brelse(zb);

        in->i_block[slot] = nb;
        in->i_blocks += fs->block_size / 512;
    }
    return map_indirect(fs, in->i_block[slot], idx, create, out);
}

int ext2_bmap(ext2_fs_t *fs, ext2_inode_t *in,
              uint32_t lblk, int create, uint32_t *out)
{
    uint32_t p = fs->block_size / 4;

    if (lblk < 12) {
        if (in->i_block[lblk]) { *out = in->i_block[lblk]; return 0; }
        if (!create) { *out = 0; return 0; }

        uint32_t nb = ext2_alloc_block(fs);
        if (!nb) return -1;

        buf_t *zb = bnew(nb, fs->block_size);
        if (!zb) { ext2_free_block(fs, nb); return -1; }
        brelse(zb);

        in->i_block[lblk] = nb;
        in->i_blocks += fs->block_size / 512;
        *out = nb;
        return 0;
    }

    uint32_t i = lblk - 12;

    if (i < p)
        return map_slot(fs, in, 12, i, create, out);
    i -= p;

    if (i < p * p) {
        uint32_t mid;
        int r = map_slot(fs, in, 13, i / p, create, &mid);
        if (r || !mid) { *out = 0; return r; }
        return map_indirect(fs, mid, i % p, create, out);
    }
    i -= p * p;

    uint32_t top;
    int r = map_slot(fs, in, 14, i / (p * p), create, &top);
    if (r || !top) { *out = 0; return r; }

    uint32_t mid;
    r = map_indirect(fs, top, (i / p) % p, create, &mid);
    if (r || !mid) { *out = 0; return r; }

    return map_indirect(fs, mid, i % p, create, out);
}

int ext2_inode_read(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *out)
{
    if (!ino || ino > fs->sb.s_inodes_count) return -1;

    uint32_t g  = (ino - 1) / fs->inodes_per_group;
    uint32_t ix = (ino - 1) % fs->inodes_per_group;

    ext2_group_desc_t *gd;
    buf_t *gb = group_desc(fs, g, &gd);
    if (!gb) return -1;

    uint32_t off   = ix * fs->inode_size;
    uint32_t blk   = gd->bg_inode_table + off / fs->block_size;
    uint32_t within = off % fs->block_size;

    buf_t *b = bread(blk, fs->block_size);
    if (!b) { brelse(gb); return -1; }

    memcpy(out, b->data + within, sizeof(ext2_inode_t));

    brelse(b);
    brelse(gb);
    return 0;
}

int ext2_inode_write(ext2_fs_t *fs, uint32_t ino, const ext2_inode_t *in)
{
    if (fs->readonly) return -1;

    uint32_t g  = (ino - 1) / fs->inodes_per_group;
    uint32_t ix = (ino - 1) % fs->inodes_per_group;

    ext2_group_desc_t *gd;
    buf_t *gb = group_desc(fs, g, &gd);
    if (!gb) return -1;

    uint32_t off   = ix * fs->inode_size;
    uint32_t blk   = gd->bg_inode_table + off / fs->block_size;
    uint32_t within = off % fs->block_size;

    buf_t *b = bread(blk, fs->block_size);
    if (!b) { brelse(gb); return -1; }

    memcpy(b->data + within, (void*)in, sizeof(ext2_inode_t));
    bdirty(b);

    brelse(b);
    brelse(gb);
    return 0;
}

int ext2_read_file(ext2_fs_t *fs, uint32_t ino, uint64_t offset, void *dst, uint32_t len)
{
    ext2_inode_t in;
    if (ext2_inode_read(fs, ino, &in)) return -1;
    if ((in.i_mode & 0xF000) != 0x8000) return -1;
    if (offset >= in.i_size) return 0;

    if (len > in.i_size - offset) len = in.i_size - (uint32_t)offset;

    uint32_t bs = fs->block_size, done = 0;
    uint8_t *out = dst;

    while (done < len) {
        uint32_t lblk   = (uint32_t)(offset / bs);
        uint32_t within = (uint32_t)(offset % bs);
        uint32_t chunk  = bs - within;
        if (chunk > len - done) chunk = len - done;

        uint32_t phys;
        if (ext2_bmap(fs, &in, lblk, 0, &phys)) return -1;

        if (!phys) {
            memset(out + done, 0, chunk);      /* sparse hole */
        } else {
            buf_t *b = bread(phys, bs);
            if (!b) return -1;
            memcpy(out + done, b->data + within, chunk);
            brelse(b);
        }

        offset += chunk;
        done   += chunk;
    }
    return (int)done;
}

uint32_t ext2_alloc_inode(ext2_fs_t *fs)
{
    if (fs->readonly) return 0;

    for (uint32_t n = 0; n < fs->groups; n++) {
        uint32_t g = (fs->hint_inode + n) % fs->groups;

        ext2_group_desc_t *gd;
        buf_t *gb = group_desc(fs, g, &gd);
        if (!gb) return 0;

        if (!gd->bg_free_inodes_count) { brelse(gb); continue; }

        buf_t *ib = bread(gd->bg_inode_bitmap, fs->block_size);
        if (!ib) { brelse(gb); return 0; }

        uint32_t found = 0xFFFFFFFF;
        for (uint32_t i = 0; i < fs->inodes_per_group; i++) {
            if (!bit_get(ib->data, i)) { found = i; break; }
        }

        if (found == 0xFFFFFFFF) { brelse(ib); brelse(gb); continue; }

        bit_set(ib->data, found, 1);
        bdirty(ib);

        gd->bg_free_inodes_count--;
        bdirty(gb);

        fs->sb.s_free_inodes_count--;
        fs->sb_dirty = 1;
        fs->hint_inode = g;

        brelse(ib);
        brelse(gb);

        return (g * fs->inodes_per_group) + found + 1;
    }
    return 0;
}

int ext2_find_file(ext2_fs_t *fs, uint32_t dir_inode_num, const char *name, uint32_t *found_inode) {
    ext2_inode_t inode;
    if (ext2_inode_read(fs, dir_inode_num, &inode) != 0) return -1;
    
    if ((inode.i_mode & 0xF000) != EXT2_S_IFDIR) return -2;
    
    uint32_t dir_size = inode.i_size;
    uint32_t bytes_read = 0;
    uint32_t name_len = (uint32_t)strlen(name);
    
    while (bytes_read < dir_size) {
        uint32_t lblk = bytes_read / fs->block_size;
        uint32_t within = bytes_read % fs->block_size;
        
        uint32_t phys;
        if (ext2_bmap(fs, &inode, lblk, 0, &phys) != 0) return -3;
        if (!phys) break; 
        
        buf_t *b = bread(phys, fs->block_size);
        if (!b) return -3;
        
        uint32_t block_offset = within;
        int found = 0;
        while (block_offset < fs->block_size) {
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(b->data + block_offset);
            if (entry->rec_len == 0) break;
            
            if (entry->name_len == name_len && memcmp(entry->name, name, name_len) == 0) {
                *found_inode = entry->inode;
                found = 1;
                break;
            }
            block_offset += entry->rec_len;
        }
        brelse(b);
        if (found) return 0;
        
        bytes_read = (bytes_read / fs->block_size + 1) * fs->block_size;
    }
    
    return -4;
}

int ext2_create_file(ext2_fs_t *fs, uint32_t dir_inode_num, const char *name, uint32_t *new_inode_num) {
    ext2_inode_t dir_inode;
    if (ext2_inode_read(fs, dir_inode_num, &dir_inode) != 0) return -1;
    if ((dir_inode.i_mode & 0xF000) != EXT2_S_IFDIR) return -2;
    if (fs->readonly) return -3;

    uint32_t free_inode = ext2_alloc_inode(fs);
    if (!free_inode) return -4;
    *new_inode_num = free_inode;

    uint32_t free_block = ext2_alloc_block(fs);
    if (!free_block) {
        ext2_free_inode(fs, free_inode);
        return -5;
    }

    ext2_inode_t new_inode;
    memset(&new_inode, 0, sizeof(ext2_inode_t));
    new_inode.i_mode = EXT2_S_IFREG | 0644;
    new_inode.i_size = 0;
    new_inode.i_links_count = 1;
    new_inode.i_blocks = fs->block_size / 512;
    new_inode.i_block[0] = free_block;

    if (ext2_inode_write(fs, free_inode, &new_inode) != 0) {
        ext2_free_block(fs, free_block);
        ext2_free_inode(fs, free_inode);
        return -6;
    }

    uint32_t dir_phys;
    if (ext2_bmap(fs, &dir_inode, 0, 0, &dir_phys) != 0 || !dir_phys) return -7;

    buf_t *dir_b = bread(dir_phys, fs->block_size);
    if (!dir_b) return -8;

    uint32_t offset = 0;
    uint32_t name_str_len = (uint32_t)strlen(name);
    uint32_t needed_len = (8 + name_str_len + 3) & ~3;

    while (offset < fs->block_size) {
        ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(dir_b->data + offset);
        if (entry->rec_len == 0) break;
       
        uint32_t real_len = (8 + entry->name_len + 3) & ~3;
        
          if (entry->rec_len >= real_len + needed_len) {
            uint32_t new_rec_len = entry->rec_len - real_len;
            entry->rec_len = real_len;
            
            ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)(dir_b->data + offset + real_len);
            new_entry->inode = free_inode;
            new_entry->rec_len = new_rec_len;
            new_entry->name_len = name_str_len;
            new_entry->file_type = 1;
            memcpy(new_entry->name, (void*)name, name_str_len);
            
            bdirty(dir_b);
            brelse(dir_b);
            
            return 0;
        }
        offset += entry->rec_len;
    }
    
    brelse(dir_b);

    ext2_free_block(fs, free_block);
    ext2_free_inode(fs, free_inode);
    return -9;
}

int ext2_write_file(ext2_fs_t *fs, uint32_t ino, uint64_t offset, const void *src, uint32_t len)
{
    if (fs->readonly) return -1;

    ext2_inode_t in;
    if (ext2_inode_read(fs, ino, &in)) return -1;
    if ((in.i_mode & 0xF000) != 0x8000) return -1;

    if (offset + len > 0xFFFFFFFFull) return -1;

    uint32_t bs = fs->block_size, done = 0;
    const uint8_t *in_buf = src;

    while (done < len) {
        uint32_t lblk   = (uint32_t)((offset + done) / bs);
        uint32_t within = (uint32_t)((offset + done) % bs);
        uint32_t chunk  = bs - within;
        if (chunk > len - done) chunk = len - done;

        uint32_t phys;
        if (ext2_bmap(fs, &in, lblk, 1, &phys)) goto fail;

        buf_t *b = (within == 0 && chunk == bs)
                 ? bnew(phys, bs)
                 : bread(phys, bs);
        if (!b) goto fail;

        memcpy(b->data + within, (void*)(in_buf + done), chunk);
        bdirty(b);
        brelse(b);

        done += chunk;
    }

    if (offset + done > in.i_size) in.i_size = (uint32_t)(offset + done);

    if (ext2_inode_write(fs, ino, &in)) return -1;
    return (int)done;

fail:
    ext2_inode_write(fs, ino, &in);
    return -1;
}

int ext2_mount(ext2_fs_t *fs)
{
    uint8_t probe[1024];
    if (ahci_read_sector(2, probe, 2) != 0) return -1;

    ext2_superblock_t *s = (ext2_superblock_t *)probe;
    if (s->s_magic != EXT2_MAGIC) return -2;

    uint32_t bs = 1024u << s->s_log_block_size;
    if (bs != 1024 && bs != 2048 && bs != 4096) return -3;

    if (s->s_feature_incompat & ~0x00000002u) return -4;
    if (s->s_feature_ro_compat & ~(0x1u | 0x2u | 0x20u | 0x40u)) return -5;
    if (s->s_feature_compat & 0x4u) return -6;
    if (!s->s_blocks_per_group || !s->s_inodes_per_group) return -7;

    uint32_t sb_block = (bs == 1024) ? 1 : 0;
    uint32_t sb_off   = (bs == 1024) ? 0 : 1024;

    buf_t *b = bread(sb_block, bs);
    if (!b) return -8;
    memcpy(&fs->sb, b->data + sb_off, sizeof(fs->sb));
    brelse(b);

    fs->block_size       = bs;
    fs->first_data       = fs->sb.s_first_data_block;
    fs->gdt_block        = sb_block + 1;
    fs->blocks_per_group = fs->sb.s_blocks_per_group;
    fs->inodes_per_group = fs->sb.s_inodes_per_group;
    fs->inode_size       = fs->sb.s_rev_level ? fs->sb.s_inode_size : 128;
    fs->hint_block       = 0;
    fs->hint_inode       = fs->sb.s_first_ino;
    fs->sb_dirty         = 0;
    fs->readonly         = 0;

    if (fs->inode_size < 128 || (fs->inode_size & 3)) return -9;

    fs->groups = (fs->sb.s_blocks_count - fs->first_data
                  + fs->blocks_per_group - 1) / fs->blocks_per_group;

    return 0;
}

int ext2_sync(ext2_fs_t *fs)
{
    if (!fs) return -1;

    if (fs->sb_dirty) {
        uint32_t sb_block = (fs->block_size == 1024) ? 1 : 0;
        uint32_t sb_off   = (fs->block_size == 1024) ? 0 : 1024;

        buf_t *b = bread(sb_block, fs->block_size);
        if (!b) return -2;

        memcpy(b->data + sb_off, &fs->sb, sizeof(ext2_superblock_t));
        bdirty(b);
        brelse(b);

        fs->sb_dirty = 0;
    }

    int errors = bsync();
    if (errors > 0) {
        fs->sb.s_state = 2; /* EXT2_ERROR_FS */
        fs->sb_dirty = 1;
        return -errors;
    }

    ahci_flush_cache();

    return 0;
}

int ext2_create_dir(ext2_fs_t *fs, uint32_t parent_inode_num, const char *name, uint32_t *new_inode_num) {
    ext2_inode_t parent_inode;
    if (ext2_inode_read(fs, parent_inode_num, &parent_inode) != 0) {
        return -1;
    }
    if ((parent_inode.i_mode & 0xF000) != EXT2_S_IFDIR) {
        return -2;
    }
    if (fs->readonly) {
        return -3;
    }

    uint32_t free_inode = ext2_alloc_inode(fs);
    if (!free_inode) {
        return -4;
    }
    *new_inode_num = free_inode;

    uint32_t free_block = ext2_alloc_block(fs);
    if (!free_block) {
        ext2_free_inode(fs, free_inode);
        return -5;
    }

    ext2_inode_t new_inode;
    memset(&new_inode, 0, sizeof(ext2_inode_t));
    new_inode.i_mode = EXT2_S_IFDIR | 0755;
    new_inode.i_size = fs->block_size;
    new_inode.i_links_count = 2;
    new_inode.i_blocks = fs->block_size / 512;
    new_inode.i_block[0] = free_block;

    if (ext2_inode_write(fs, free_inode, &new_inode) != 0) {
        ext2_free_block(fs, free_block);
        ext2_free_inode(fs, free_inode);
        return -6;
    }

    buf_t *dir_b = bnew(free_block, fs->block_size);
    if (!dir_b) {
        ext2_free_block(fs, free_block);
        ext2_free_inode(fs, free_inode);
        return -7;
    }

    ext2_dir_entry_t *dot = (ext2_dir_entry_t *)dir_b->data;
    dot->inode = free_inode;
    dot->rec_len = 12;
    dot->name_len = 1;
    dot->file_type = 2;  // EXT2_FT_DIR
    dot->name[0] = '.';
    dot->name[1] = '\0';
    
    ext2_dir_entry_t *dotdot = (ext2_dir_entry_t *)(dir_b->data + 12);
    dotdot->inode = parent_inode_num;
    dotdot->rec_len = fs->block_size - 12;
    dotdot->name_len = 2;
    dotdot->file_type = 2;  // EXT2_FT_DIR
    dotdot->name[0] = '.';
    dotdot->name[1] = '.';
    dotdot->name[2] = '\0';
    
    bdirty(dir_b);
    brelse(dir_b);
    
    uint32_t parent_phys;
    if (ext2_bmap(fs, &parent_inode, 0, 0, &parent_phys) != 0 || !parent_phys) {
        ext2_free_block(fs, free_block);
        ext2_free_inode(fs, free_inode);
        return -8;
    }
    
    buf_t *parent_b = bread(parent_phys, fs->block_size);
    if (!parent_b) {
        ext2_free_block(fs, free_block);
        ext2_free_inode(fs, free_inode);
        return -9;
    }
    
    uint32_t offset = 0;
    uint32_t name_str_len = (uint32_t)strlen(name);
    uint32_t needed_len = (8 + name_str_len + 3) & ~3;
    
    while (offset < fs->block_size) {
        ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(parent_b->data + offset);
        if (entry->rec_len == 0) break;
        
        if (entry->rec_len >= needed_len + 8) {
            uint32_t new_rec_len = entry->rec_len - needed_len;
            entry->rec_len = needed_len;
            
            ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)(parent_b->data + offset + needed_len);
            new_entry->inode = free_inode;
            new_entry->rec_len = new_rec_len;
            new_entry->name_len = name_str_len;
            new_entry->file_type = 2;  // EXT2_FT_DIR
            memcpy(new_entry->name, (void*)name, name_str_len);
            
            bdirty(parent_b);
            brelse(parent_b);
            
            return 0;
        }
        offset += entry->rec_len;
    }
    
    brelse(parent_b);
    ext2_free_block(fs, free_block);
    ext2_free_inode(fs, free_inode);
    return -10;
}

int ext2_readdir(ext2_fs_t *fs, uint32_t dir_inode_num, uint32_t index, vfs_dirent_t *dirent)
{
    ext2_inode_t dir_inode;
    if (ext2_inode_read(fs, dir_inode_num, &dir_inode) != 0) {
        return -1;
    }
    
    if ((dir_inode.i_mode & 0xF000) != EXT2_S_IFDIR) {
        return -2;
    }
    
    uint32_t dir_size = dir_inode.i_size;
    uint32_t bytes_read = 0;
    uint32_t current_index = 0;
    
    while (bytes_read < dir_size) {
        uint32_t lblk = bytes_read / fs->block_size;
        uint32_t within = bytes_read % fs->block_size;
        
        uint32_t phys;
        if (ext2_bmap(fs, &dir_inode, lblk, 0, &phys) != 0) {
            return -3;
        }
        if (!phys) break;
        
        buf_t *b = bread(phys, fs->block_size);
        if (!b) return -3;
        
        uint32_t block_offset = within;
        while (block_offset < fs->block_size) {
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(b->data + block_offset);
            if (entry->rec_len == 0) break;
            
            if (entry->name_len == 1 && entry->name[0] == '.') {
                block_offset += entry->rec_len;
                continue;
            }
            if (entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.') {
                block_offset += entry->rec_len;
                continue;
            }
            
            if (current_index == index) {
                strncpy(dirent->name, entry->name, entry->name_len);
                dirent->name[entry->name_len] = '\0';
                dirent->inode = entry->inode;
                
                if (entry->file_type == 2) {  // EXT2_FT_DIR
                    dirent->flags = VFS_FLAG_DIRECTORY;
                } else {
                    dirent->flags = VFS_FLAG_FILE;
                }
                
                brelse(b);
                return 0;
            }
            
            current_index++;
            block_offset += entry->rec_len;
        }
        
        brelse(b);
        bytes_read = (bytes_read / fs->block_size + 1) * fs->block_size;
    }
    
    return -1;
}

vfs_node_t *ext2_vfs_finddir(vfs_node_t *node, const char *name) 
{
    ext2_fs_t *fs = (ext2_fs_t *)node->device;
    uint32_t found_ino = 0;
    
    if (ext2_find_file(fs, node->inode, name, &found_ino) != 0) {
        return 0;
    }

    ext2_inode_t in;
    if (ext2_inode_read(fs, found_ino, &in) != 0) {
        return 0;
    }

    vfs_node_t *new_node = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    memset(new_node, 0, sizeof(vfs_node_t));
    
    strncpy(new_node->name, name, sizeof(new_node->name));
    new_node->inode = found_ino;
    new_node->size = in.i_size;
    new_node->parent = node;
    new_node->device = fs;
    
    if ((in.i_mode & 0xF000) == 0x4000) { // EXT2_S_IFDIR
        new_node->flags = VFS_FLAG_DIRECTORY;
    } else {
        new_node->flags = VFS_FLAG_FILE;
    }

    new_node->ops = (vfs_ops_t *)malloc(sizeof(vfs_ops_t));
    new_node->ops->read = ext2_vfs_read;
    new_node->ops->write = ext2_vfs_write;
    new_node->ops->finddir = ext2_vfs_finddir;
    new_node->ops->create = ext2_vfs_create;
    new_node->ops->mkdir = ext2_vfs_mkdir;
    new_node->ops->readdir = ext2_vfs_readdir;

    return new_node;
}

int ext2_vfs_read(vfs_node_t *node, uint64_t offset, uint32_t size, char *buffer) 
{
    ext2_fs_t *fs = (ext2_fs_t *)node->device;
    return ext2_read_file(fs, node->inode, offset, buffer, size);
}

int ext2_vfs_write(vfs_node_t *node, uint64_t offset, uint32_t size, const char* buffer)
{
    ext2_fs_t *fs = (ext2_fs_t *)node->device;
    return ext2_write_file(fs, node->inode, offset, buffer, size);
}

int vfs_mount_ext2_root(void) 
{
    ext2_fs_t *fs = (ext2_fs_t *)malloc(sizeof(ext2_fs_t));
    memset(fs, 0, sizeof(ext2_fs_t));
    
    int res = ext2_mount(fs);
    if (res != 0) {
        free(fs);
        return res;
    }

    vfs_node_t *ext2_root = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    memset(ext2_root, 0, sizeof(vfs_node_t));
    strcpy(ext2_root->name, "/");
    ext2_root->flags = VFS_FLAG_DIRECTORY;
    ext2_root->inode = 2;
    ext2_root->device = fs;
    ext2_root->parent = ext2_root;
    
    ext2_root->ops = (vfs_ops_t *)malloc(sizeof(vfs_ops_t));
    ext2_root->ops->read = ext2_vfs_read;
    ext2_root->ops->write = ext2_vfs_write;
    ext2_root->ops->finddir = ext2_vfs_finddir;
    ext2_root->ops->create = ext2_vfs_create;
    ext2_root->ops->mkdir = ext2_vfs_mkdir;
    ext2_root->ops->readdir = ext2_vfs_readdir;

    free(vfs_root);
    vfs_root = ext2_root;
    vfs_cwd = ext2_root;

    return 0;
}

vfs_node_t *ext2_vfs_create(vfs_node_t *parent, const char *name) 
{
    ext2_fs_t *fs = (ext2_fs_t *)parent->device;
    uint32_t new_inode = 0;
    
    if (ext2_create_file(fs, parent->inode, name, &new_inode) != 0) {
        return NULL;
    }
    
    vfs_node_t *new_node = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    memset(new_node, 0, sizeof(vfs_node_t));
    strncpy(new_node->name, name, sizeof(new_node->name));
    new_node->inode = new_inode;
    new_node->flags = VFS_FLAG_FILE;
    new_node->size = 0;
    new_node->parent = parent;
    new_node->device = fs;
    
    new_node->ops = (vfs_ops_t *)malloc(sizeof(vfs_ops_t));
    new_node->ops->read = ext2_vfs_read;
    new_node->ops->write = ext2_vfs_write;
    new_node->ops->finddir = ext2_vfs_finddir;
    new_node->ops->create = ext2_vfs_create;
    new_node->ops->mkdir = ext2_vfs_mkdir;
    new_node->ops->readdir = ext2_vfs_readdir;
    
    return new_node;
}

vfs_node_t *ext2_vfs_mkdir(vfs_node_t *parent, const char *name) 
{
    ext2_fs_t *fs = (ext2_fs_t *)parent->device;
    uint32_t new_inode = 0;
    
    if (ext2_create_dir(fs, parent->inode, name, &new_inode) != 0) {
        return NULL;
    }
    
    vfs_node_t *new_node = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    memset(new_node, 0, sizeof(vfs_node_t));
    strncpy(new_node->name, name, sizeof(new_node->name));
    new_node->inode = new_inode;
    new_node->flags = VFS_FLAG_DIRECTORY;
    new_node->size = fs->block_size;
    new_node->parent = parent;
    new_node->device = fs;
    
    new_node->ops = (vfs_ops_t *)malloc(sizeof(vfs_ops_t));
    memset(new_node->ops, 0, sizeof(vfs_ops_t));
    new_node->ops->read = ext2_vfs_read;
    new_node->ops->write = ext2_vfs_write;
    new_node->ops->finddir = ext2_vfs_finddir;
    new_node->ops->create = ext2_vfs_create;
    new_node->ops->mkdir = ext2_vfs_mkdir;
    new_node->ops->readdir = ext2_vfs_readdir;
    
    return new_node;
}

int ext2_vfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    ext2_fs_t *fs = (ext2_fs_t *)node->device;
    return ext2_readdir(fs, node->inode, index, dirent);
}