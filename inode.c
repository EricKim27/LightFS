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
    struct inode *mem_inode = iget_locked(sb, inode);
    mem_inode->i_ino = inode;
    mem_inode->i_sb = sb;
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
    mem_inode->i_size = raw_inode->i_size;
    mem_inode->i_mode = raw_inode->i_mode;
    mem_inode->__i_atime = raw_inode->i_atime;
    mem_inode->__i_mtime = raw_inode->i_mtime;
    mem_inode->__i_ctime = raw_inode->i_ctime;
     if (S_ISDIR(mem_inode->i_mode)) {
        //TODO: define operations
     } else if(S_ISREG(mem_inode->i_mode)){
        //TODO: define operations
     } else if(S_ISLINK(mem_inode->i_mode)){
        //TODO: define operations
     }
    brelse(bh);
    unlock_new_inode(inode);

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
    char *block = (char *)get_block(sb, );
}

