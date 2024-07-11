#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/dcache.h>

struct inode_operations lightfs_inode_operations;

//getting inode structure from disk
struct inode *lightfs_iget(struct super_block *sb, __u64 inode)
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
    raw_inode = (struct lightfs_inode *)(bh->b_data + inode_location_inlb);
    //TODO: fill the memory inode structure with raw inode structure.
    i_gid_write(mem_inode, raw_inode->i_gid);
    i_uid_write(mem_inode, raw_inode->i_uid);
    mem_inode->i_size = le32_to_cpu(raw_inode->i_size);
    mem_inode->i_mode = le32_to_cpu(raw_inode->i_mode);
    mem_inode->__i_atime = raw_inode->i_atime;
    mem_inode->__i_mtime = raw_inode->i_mtime;
    mem_inode->__i_ctime = raw_inode->i_ctime;
    ci->block = &raw_inode->block;//This would not work. Need to figure out a way to do it.
    ci->d_ind_blk = &raw_inode->d_ind_blk;
    ci->ind_blk = &raw_inode->ind_blk;

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
    struct super_block *sb = dir->i_sb;
    struct lightfs_inode_info *ci = dir->i_private;
    
    unsigned int i;
    char *buf;
    for(i=0, i<ci->blocks; i++)
    {
        if(i<12)
        {
            buf = (char *)get_block(sb, ci->block[i]);
            //TODO: Think of a block buffering mechanism
        }
    }
    char *block = (char *)get_block(sb, ci->block[0]);
}
