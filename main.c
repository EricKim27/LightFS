#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "lightfs.h"

int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head *sbh = NULL;
    struct lightfs_superblock *chksb = NULL;
    struct lightfs_superblock *sbi = NULL;
    struct inode *root_inode = NULL;
    int ret = 0;

    sb->s_magic = lightfs_magic;
    sb_set_block_size(sb, 1024);
    sbh = sb_bread(sb, 1);
    if(!sbh)
        return -EIO;
    
    chksb = (struct lightfs_superblock *) sbh->b_data;

    if(chksb->magicsig != sb->s_magic)
    {
        ret = 1;
        goto release;
    }
    sbi = kzalloc(sizeof(struct lightfs_superblock), GFP_KERNEL);

    if(!sbi)
    {
        ret = -ENOMEM;
        goto release;
    }

    if(chksb->error != 0)
    {
        printk(KERN_ERR "Error was detected during fs mount. check filesystem.");
        ret = -EIO;
        goto release;
    }
    sbi->block_size = chksb->block_size;
    sbi->created_os = chksb->created_os;
    sbi->data_block_num = chksb->data_block_num;
    sbi->error = 1;
    
    return ret;
release:
    if(ret)
        kfree(sbi);
    brelse(sbh);
    return ret;
}
