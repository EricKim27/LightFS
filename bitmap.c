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

int lightfs_get_first_bit(struct super_block *sb)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    bool *bmap_cursor;
    bmap_cursor = (bool *)(sbi->data_bitmap);
    unsigned int i;
    for(i = 0; i<sbi->data_block_num; i++)
    {
        if(*bmap_cursor == 1) {
            return i;
        }
        bmap_cursor++;
    }
    return -ENOENT;
}

int change_ibitmap(struct super_block *sb, __u32 ino)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh;
    __u32 bitmap_offset;
    __u32 bitmap_shift;
    bool *bitmap;

    bitmap_shift = ino % LIGHTFS_LOGICAL_BS;
    if(bitmap_shift == 0)
        bitmap_offset = 1 + (ino / LIGHTFS_LOGICAL_BS);
    else
        bitmap_offset = 1 + (ino / LIGHTFS_LOGICAL_BS) + 1;
    
    bh = sb_bread(sb, bitmap_offset);
    if(!bh) {
        return -EFAULT;
    }
    bitmap = (bool *)((char *)bh->b_data + bitmap_shift);
    if(*bitmap == true)
        *bitmap = false;
    else
        *bitmap = true;

    mark_buffer_dirty(bh);
    brelse(bh);
    return 0;
}

int change_bbitmap(struct super_block *sb, __u32 blk)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh;
    __u32 block_offset;
    __u32 block_shift = blk % LIGHTFS_LOGICAL_BS;
    bool *bitmap;

    if(block_shift == 0) {
        block_offset = ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + (block_offset / LIGHTFS_LOGICAL_BS);
    } else {
        block_offset = ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + (block_offset / LIGHTFS_LOGICAL_BS) + 1;
    }
    bh = sb_bread(sb, block_offset);
    if(!bh) {
        return -EFAULT;
    }
    bitmap = (bool *)((char *)bh->b_data + block_shift);
    if(*bitmap == 1)
        *bitmap = false;
    else
        *bitmap = true;
        
    mark_buffer_dirty(bh);
    brelse(bh);
    return 0;
}