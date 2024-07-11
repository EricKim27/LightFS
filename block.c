#include "lightfs.h"
#include <linux/buffer_head.h>
#include <linux/slab.h>

void *get_block(struct super_block *sb, __u32 num)
{
    struct buffer_head *bh = NULL;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    int logical_per_physical = (sbi->block_size / LIGHTFS_LOGICAL_BS);
    unsigned int db_offset = 1 + sbi->data_block_num + sbi->inode_block_num;//Not a full calculation method
    //TODO: think of a offset calculating mechanism

    void *buf = kmalloc(sbi->block_size, GFP_KERNEL);
    if(!buf)
    {
        return -ENOMEM;
    }
    unsigned int i;

    for(i = 0; i<logical_per_physical; i++)
    {
        bh = sb_bread(sb, db_offset+i);

        if(!bh)
        {
            kfree(buf);
            return -EIO;
        }
        memcpy(buf + (i * LIGHTFS_LOGICAL_BS), bh->b_data, LIGHTFS_LOGICAL_BS);
        brelse(bh);
    }

    return buf;
}