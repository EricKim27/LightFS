#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>

//getting bitmap and connecting it to the superblock structure. Used when filling superblock structure.
int lightfs_get_bitmap(struct super_block *sb)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh = NULL;
    sbi->inode_bmap = kmalloc(sbi->inode_block_num, GFP_KERNEL);
    sbi->data_bitmap = kmalloc(sbi->data_block_num, GFP_KERNEL);

    unsigned int d_breadno = (sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1;
    unsigned int i_breadno = (sbi->inode_block_num / LIGHTFS_LOGICAL_BS) + 1;
    unsigned int i_bmap_offset = 2 + d_breadno;
    unsigned int i;
    //read datablock bitmap TODO: Think of a offset calculation method.
    for(i = 0; i < d_breadno; i++)
    {
        bh = sb_bread(sb, 2+i);
        if(!bh)
        {
            printk(KERN_ERR "Read Error during reading bitmap: failed to read data bitmap.\n");
            goto error;
        }
        memcpy(sbi->data_bitmap + (i * LIGHTFS_LOGICAL_BS), bh->b_data, LIGHTFS_LOGICAL_BS);
        brelse(bh);
    }
    for(i = 0; i < i_breadno; i++)
    {
        bh = sb_bread(sb, i_bmap_offset+i);
        if(!bh)
        {
            printk(KERN_ERR "Read Error during reading bitmap: failed to read inode bitmap.\n");
            goto error;
        }
        memcpy(sbi->inode_bmap + (i * LIGHTFS_LOGICAL_BS), bh->b_data, LIGHTFS_LOGICAL_BS);
        brelse(bh);
    }
    return 0;
error:
    kfree(sbi->inode_bmap);
    kfree(sbi->data_bitmap); 
    return -EIO;
}

//free bitmap structure. Used during umount job.
void lightfs_free_bitmap(struct super_block *sb)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    kfree(sbi->inode_bmap);
    kfree(sbi->data_bitmap);
}