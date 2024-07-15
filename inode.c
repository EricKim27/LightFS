#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/dcache.h>
#include <linux/string.h>

struct inode_operations lightfs_inode_operations;

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
    ci->block = kmalloc(sizeof(raw_inode->block));
    ci->d_ind_blk = kmalloc(sizeof(raw_inode->d_ind_blk));
    ci->ind_blk = kmalloc(sizeof(raw_inode->ind_blk));
    memcpy(ci->block, raw_inode->block, sizeof(raw_inode->block));
    memcpy(ci->d_ind_blk, raw_inode->d_ind_blk, sizeof(raw_inode->d_ind_blk));
    memcpy(ci->ind_blk, raw_inode->ind_blk, sizeof(raw_inode->ind_blk));

     if (S_ISDIR(mem_inode->i_mode)) {
        //TODO: define operations
     } else if(S_ISREG(mem_inode->i_mode)){
        //TODO: define operations
     } else if(S_ISLNK(mem_inode->i_mode)){
        //TODO: define operations
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
/*
int simplefs_create(struct mnt_idmap *id,
                       struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
    struct lightfs_dentry *dentry = NULL;
    struct super_block *sb = dir->i_sb;
    struct inode *inode;
    struct lightfs_inode *inode_i;
    struct lightfs_inode_info *ii;
    struct buffer_head **bh;
    struct lightfs_d_head *dh;
    inode = new_inode(sb);
    unsigned int i;

    inode->i_ino = get_next_ino();
    inode_init_owner(id, inode, dir, mode);
    inode->i_size = 0;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    //TODO: set operations

    bh = get_block(sb, ii->block[0]);
    dh = (struct lightfs_d_head *)bh[0]->b_data;
    size_t new_dentry_offset = (dh->item_num * sizeof(struct lightfs_dentry));//TODO: think of a way to calculate the offset
}
*/
