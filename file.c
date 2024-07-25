#include "lightfs.h"
#include <linux/buffer_head.h>
#include <linux/slab.h>

/* plans to change this function: return a pointer to buffer containing data
 * and then later use sync_block to sync the data to the disk.
 */
char *get_block(struct super_block *sb, __u32 num)
{
    struct buffer_head **bh = NULL;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    int logical_per_physical = (sbi->block_size / LIGHTFS_LOGICAL_BS);
    size_t inode_bmap_no;
    size_t data_bmap_no;
    size_t inode_lb_no;

    inode_bmap_no = sbi->inode_block_num / LIGHTFS_LOGICAL_BS + 1;
    data_bmap_no = sbi->data_block_num / LIGHTFS_LOGICAL_BS + 1;

    if((sbi->inode_block_num % 4) == 0) {
        inode_lb_no = sbi->inode_block_num / 4;
    } else {
        inode_lb_no = sbi->inode_block_num / 4 + 1;
    }

    size_t db_offset = 1 + data_bmap_no + inode_bmap_no + inode_lb_no + (num * 4);
    //TODO: validate the calculation method

    char *buf = (char *)kmalloc(sbi->block_size, GFP_KERNEL); 
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
    buf = blkcpy(bh, sbi);

    return buf;
}

char *blkcpy(struct buffer_head **bh, struct lightfs_superblock *sbi)
{
    char *buf=(char *)kmalloc(sbi->block_size, GFP_KERNEL);
    unsigned int i;

    for(i=0; i<(sbi->block_size / LIGHTFS_LOGICAL_BS); i++){
        memcpy(buf+(i*1024), bh[i]->b_data, LIGHTFS_LOGICAL_BS);
        printk(KERN_INFO "Copying logical block %d to buffer\n", i);
    }
    printk(KERN_INFO "Copy of one block complete\n");
    for(i=0; i<(sbi->block_size / LIGHTFS_LOGICAL_BS); i++){
        brelse(bh[i]);
    }

    return buf;
}

int sync_block(struct super_block *sb, __u32 block_no, char *buf)
{
    if (!sb || !block_no || !buf) return -EINVAL;

    unsigned int i;
    struct buffer_head **bh;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    bh = get_block_bh(sb, block_no);
    if(!bh)
    {
        return -EIO;
    }
    
    for(i=0; i<(sbi->block_size / LIGHTFS_LOGICAL_BS); i++) {

        memcpy(bh[i]->b_data, buf+(i*1024), LIGHTFS_LOGICAL_BS);
        printk(KERN_INFO "syncing logical block %d with disk\n", i);

        mark_buffer_dirty(bh[i]);
        brelse(bh[i]);
    }
    kfree(buf);
    return 0;
}

//additional cleanup job for block, soon to change & deprecated
void block_cleanup(struct buffer_head **bh, struct lightfs_superblock *sbi)
{
    int blk_num = sbi->block_size / LIGHTFS_LOGICAL_BS;
    unsigned int i;
    for(i = 0; i<blk_num; i++) {
        mark_buffer_dirty(bh[i]);
        brelse(bh[i]);
    }
}

static int *lightfs_open(struct inode *inode, struct file *file)
{
    struct lightfs_inode_info *i_info = inode->i_private;
    /*
     - How this operation will work

     It's going to look for the file's data in the cache, and if there's no entry,
     And update the in-memory inode structure to inform that this file is open.
     Then, it'll check the permission to see if the person trying to access it has permission.
     And it'll return 0 if successful.
    */
    return 0;
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
struct buffer_head **get_block_bh(struct super_block *sb, __u32 num)
{
    struct buffer_head **bh = NULL;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    int logical_per_physical = (sbi->block_size / LIGHTFS_LOGICAL_BS);
    size_t inode_bmap_no;
    size_t data_bmap_no;
    size_t inode_lb_no;

    inode_bmap_no = sbi->inode_block_num / LIGHTFS_LOGICAL_BS + 1;
    data_bmap_no = sbi->data_block_num / LIGHTFS_LOGICAL_BS + 1;

    if((sbi->inode_block_num % 4) == 0) {
        inode_lb_no = sbi->inode_block_num / 4;
    } else {
        inode_lb_no = sbi->inode_block_num / 4 + 1;
    }

    size_t db_offset = 1 + data_bmap_no + inode_bmap_no + inode_lb_no + (num * 4);
    //TODO: validate the calculation method

    unsigned int i;

    for(i = 0; i<logical_per_physical; i++)
    {
        bh[i] = sb_bread(sb, db_offset+i);

        if(!bh)
        {
            return NULL;
        }
    }
    return bh;
}