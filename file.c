#include "lightfs.h"
#include <linux/buffer_head.h>
#include <linux/slab.h>

struct buffer_head **get_block(struct super_block *sb, __u32 num)
{
    struct buffer_head **bh = NULL;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    int logical_per_physical = (sbi->block_size / LIGHTFS_LOGICAL_BS);
    size_t inode_bmap_no;
    size_t data_bmap_no;
    size_t inode_lb_no;

    if((sbi->inode_block_num % LIGHTFS_LOGICAL_BS) == 0) {
        inode_bmap_no = sbi->inode_block_num / LIGHTFS_LOGICAL_BS;
    } else {
        inode_bmap_no = sbi->inode_block_num / LIGHTFS_LOGICAL_BS + 1;
    }

    if((sbi->data_block_num % LIGHTFS_LOGICAL_BS) == 0) {   
        data_bmap_no = sbi->data_block_num / LIGHTFS_LOGICAL_BS;
    } else {
        data_bmap_no = sbi->data_block_num / LIGHTFS_LOGICAL_BS + 1;
    }

    if((sbi->inode_block_num % 4) == 0) {
        inode_lb_no = sbi->inode_block_num / 4;
    } else {
        inode_lb_no = sbi->inode_block_num / 4 + 1;
    }

    size_t db_offset = 1 + data_bmap_no + inode_bmap_no + inode_lb_no + (num * 4);
    //TODO: validate the calculation method

    void *buf = kmalloc(sbi->block_size, GFP_KERNEL); 
    if(!buf)
    {
        return NULL;
    }
    unsigned int i;

    for(i = 0; i<logical_per_physical; i++)
    {
        bh[i] = sb_bread(sb, db_offset+i);

        if(!bh)
        {
            kfree(buf);
            return NULL;
        }
    }

    return bh;
}
static ssize_t lightfs_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file->f_mapping->host;
    struct address_space *mapping = inode->i_mapping;
    struct buffer_head *bh;
    char *kbuf;
    ssize_t ret = 0;
    loff_t pos = *ppos;
    loff_t size = i_size_read(inode);
    //TODO: think of a way to read 4 logical blocks

    return 0;
}