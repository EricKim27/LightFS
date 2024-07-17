#include "lightfs.h"
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/dcache.h>
#include <linux/string.h>

struct inode_operations lightfs_inode_operations;

static const struct file_operations lightfs_file_operations;
static const struct file_operations lightfs_link_operations;
static const struct file_operations lightfs_dir_operations;

//getting inode structure from disk
struct inode *lightfs_iget(struct super_block *sb, size_t inode)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh = NULL;
    struct lightfs_inode *raw_inode = NULL;
    struct lightfs_inode_info *ci = NULL;
    struct inode *mem_inode = iget_locked(sb, inode);
    ci = kmalloc(sizeof(struct lightfs_inode_info), GFP_KERNEL);
    mem_inode->i_ino = inode;
    mem_inode->i_sb = sb;
    mem_inode->i_private = ci;
    //TODO: Verify if the calculated offset is correct
    __u32 inode_offset = 1 + ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + ((sbi->inode_block_num / LIGHTFS_LOGICAL_BS) + 1) + (inode / LIGHTFS_LOGICAL_BS);
    unsigned int inode_location_inlb = inode % LIGHTFS_LOGICAL_BS;
    bh = sb_bread(sb, inode_offset);

    if(!bh)
    {
        printk(KERN_ERR "Error while reading inode,\n");
        goto error;
    }
    raw_inode = (struct lightfs_inode *)(bh->b_data + (inode_location_inlb * sizeof(struct lightfs_inode)));
    i_gid_write(mem_inode, raw_inode->i_gid);
    i_uid_write(mem_inode, raw_inode->i_uid);
    mem_inode->i_size = le32_to_cpu(raw_inode->i_size);
    mem_inode->i_mode = le32_to_cpu(raw_inode->i_mode);
    mem_inode->__i_atime = raw_inode->i_atime;
    mem_inode->__i_mtime = raw_inode->i_mtime;
    mem_inode->__i_ctime = raw_inode->i_ctime;
    ci->block = kmalloc(sizeof(raw_inode->block), GFP_KERNEL);
    ci->d_ind_blk = kmalloc(sizeof(raw_inode->d_ind_blk), GFP_KERNEL);
    ci->ind_blk = kmalloc(sizeof(raw_inode->ind_blk), GFP_KERNEL);
    memcpy(ci->block, raw_inode->block, sizeof(raw_inode->block));
    memcpy(ci->d_ind_blk, raw_inode->d_ind_blk, sizeof(raw_inode->d_ind_blk));
    memcpy(ci->ind_blk, raw_inode->ind_blk, sizeof(raw_inode->ind_blk));

     if (S_ISDIR(mem_inode->i_mode)) {
        //TODO: define operations
        mem_inode->i_fop = &lightfs_dir_operations;
     } else if(S_ISREG(mem_inode->i_mode)){
        //TODO: define operations
        mem_inode->i_fop = &lightfs_file_operations;
     } else if(S_ISLNK(mem_inode->i_mode)){
        //TODO: define operations
        mem_inode->i_fop = &lightfs_link_operations;
     }
    brelse(bh);
    unlock_new_inode(mem_inode);

    return mem_inode;
error:
    brelse(bh);
    return NULL;
}
struct dentry *lightfs_lookup(struct inode *dir,
                            struct dentry *dentry,
                            unsigned int flags)
{
    struct dentry *found_dentry;
    struct super_block *sb = dir->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct lightfs_inode_info *ci = dir->i_private;
    struct lightfs_dentry *dentry_info = NULL;

    found_dentry = d_lookup(dentry, &dentry->d_name);
    if(found_dentry) {
        return found_dentry;
    }
    
    unsigned int i;
    unsigned int j;
    void *raw_dir = kmalloc(ci->blocks * sbi->block_size, GFP_KERNEL);
    if(!raw_dir)
    {
        printk(KERN_ERR "Error while allocating area for dentry_lookup: line 77 @ inode.c\n");
        kfree(raw_dir);
        return NULL;
    }
    //directory block has the maximum block of 12 blocks.
    for(i=0; i<ci->blocks && i<12; i++)
    {
        struct buffer_head **bh = NULL;
        bh = get_block(sb, ci->block[i]);
        void *blkbuf = kmalloc(sizeof(sbi->block_size), GFP_KERNEL);
        if(bh == NULL)
        {
            printk(KERN_ERR "Error while get_block() function: line 87 @ inode.c\n");
        }

        for(j=0; j<(sbi->block_size / LIGHTFS_LOGICAL_BS); j++)
        {
            memcpy(blkbuf + (j*sbi->block_size), bh[i]->b_data + (i * LIGHTFS_LOGICAL_BS), LIGHTFS_LOGICAL_BS);
        }

        memcpy(raw_dir + (i*sbi->block_size), blkbuf, sbi->block_size);
        memset(blkbuf, 0, sbi->block_size);

        for(j=0; j<(sbi->block_size / LIGHTFS_LOGICAL_BS); j++)
        {
            brelse(bh[j]);
        }
    }

    dentry_info = (struct lightfs_dentry *)(raw_dir + sizeof(struct lightfs_d_head));
    size_t num_entries = (ci->blocks * sbi->block_size) / sizeof(struct lightfs_dentry);
    for(i=0; i<num_entries; i++)
    {
        if(strncmp(dentry_info->filename, dentry->d_name.name, sizeof(dentry_info->filename)))
        {
            //TODO: return dentry job
            
            kfree(raw_dir);
            return found_dentry;
        }
        dentry_info++;
    }
    kfree(raw_dir);
    return NULL;
}

int lightfs_create(struct mnt_idmap *id,
                       struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
    struct lightfs_dentry *dentry_location = NULL;
    struct lightfs_dentry *dentry_from = kmalloc(sizeof(struct lightfs_dentry), GFP_KERNEL);
    struct super_block *sb = dir->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct inode *inode;
    struct lightfs_inode *inode_i;
    struct lightfs_inode_info *ii;
    struct buffer_head **bh;
    struct buffer_head *ibh;
    struct buffer_head *bbh;
    struct lightfs_d_head *dh;
    inode = new_inode(sb);
    unsigned int i;

    inode->i_ino = get_next_ino();
    inode_init_owner(id, inode, dir, mode);
    inode->i_size = 0;
    inode->__i_atime = inode->__i_mtime = inode->__i_ctime = current_time(inode);
    
    size_t b_offset = 1 + (inode->i_ino / LIGHTFS_LOGICAL_BS) + 1;
    size_t b_shift = inode->i_ino % LIGHTFS_LOGICAL_BS;
    bbh = sb_bread(sb, b_offset);
    bool *bmap_mark = (bool *)(bbh->b_data) + b_shift;
    *bmap_mark = 1;

    size_t i_offset = 1 + ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + ((sbi->inode_block_num / LIGHTFS_LOGICAL_BS) + 1) + (inode->i_ino / 4) + 1;
    size_t i_shift = inode->i_ino % 4;
    ibh = sb_bread(sb, i_offset);
    inode_i = (struct lightfs_inode *)(ibh->b_data) + i_shift;

    //TODO: set operations
    inode_i->i_atime = inode_i->i_mtime = inode_i->i_ctime = inode->__i_atime;
    inode_i->i_mode = inode->i_mode;
    inode_i->i_gid = inode->i_gid.val;
    inode_i->i_uid = inode->i_uid.val;
    inode_i->i_size = inode->i_size;
    memcpy(inode_i->block, ii->block, sizeof(__u32) * 12);
    memcpy(inode_i->ind_blk, ii->ind_blk, sizeof(__u32) * 4);
    memcpy(inode_i->d_ind_blk, ii->d_ind_blk, sizeof(__u32) * 2);

    //add entry to block
    strncpy(dentry_from->filename, dentry->d_name.name, sizeof(dentry_from->filename) - 1);
    dentry_from->filename[sizeof(dentry_from->filename) - 1] = '\0';
    dentry_from->inode = dentry->d_inode->i_ino;
    bh = get_block(sb, ii->block[0]);
    dh = (struct lightfs_d_head *)bh[0]->b_data;

    size_t block_num = dh->item_num+1 / 64;
    size_t block_shift = dh->item_num+1 % 64;

    if(block_shift == 0) {
        bh = get_block(sb, ii->block[block_num-1]);

    } else {
        bh = get_block(sb, ii->block[block_num]);
    }
    if(!bh) {
        printk(KERN_ERR "Error at line 163 @ inode.c\n");
        return -EIO;
    }
    
    size_t dentry_tail = (dh->item_num+1) - 64 * (block_num - 1);
    size_t lb_num = block_shift / 14;
    size_t lb_shift = block_shift % 14;
    dentry_location = &((struct lightfs_dentry *)bh[lb_num]->b_data)[lb_shift+1];
    
    memcpy(dentry_location, dentry_from, sizeof(struct lightfs_dentry));


    for(i = 0; i<4; i++)
    {
        mark_buffer_dirty(bh[i]);
    }
    mark_buffer_dirty(ibh);
    mark_buffer_dirty(bbh);

    for(i = 0; i<4; i++)
    {
        brelse(bh[i]);
    }
    brelse(ibh);
    brelse(bbh);

    kfree(dentry_from);
    return 0;
}
int lightfs_mkdir(struct mnt_idmap *id, struct inode *dir, struct dentry *dentry, umode_t mode)
{
    return lightfs_create(id, dir, dentry, mode | S_IFDIR, 0);
}