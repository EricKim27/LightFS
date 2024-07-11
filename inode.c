#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/dcache.h>

struct inode *lightfs_iget(struct super_block *sb, __u64 inode)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh = NULL;
    struct lightfs_inode *raw_inode = NULL;
    struct inode *mem_inode = NULL;

    //TODO: Verify if the calculated offset is correct
    __u32 inode_offset = 1 + sbi->data_block_num + sbi->inode_block_num + (inode / LIGHTFS_LOGICAL_BS);
    unsigned int inode_location_inlb = inode % LIGHTFS_LOGICAL_BS;
    bh = sb_bread(sb, inode_offset);

    if(!bh)
    {
        printk(KERN_ERR "Error while reading inode,\n");
        goto error;
    }

    raw_inode = (struct lightfs_inode *)(bh->b_data + inode_location_inlb);
    //TODO: fill the memory inode structure with raw inode structure.
    brelse(bh);
    return mem_inode;
error:
    brelse(bh);
    return NULL;
}