#include <linux/fs.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include "lightfs.h"

const struct super_operations lightfs_s_ops = {
    .put_super = lightfs_put_super,
    .statfs = lightfs_statfs,
    .sync_fs = lightfs_syncfs,
};

int lightfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head *sbh = NULL;
    struct lightfs_superblock *chksb = NULL;
    struct lightfs_superblock *sbi = NULL;
    struct inode *root_inode = NULL;
    int ret = 0;

    sb->s_op = &lightfs_s_ops;
    sb->s_magic = lightfs_magic;
    sb_set_blocksize(sb, 1024);
    sbh = sb_bread(sb, 1);
    if(!sbh)
        return -EIO;
    
    chksb = (struct lightfs_superblock *) sbh->b_data;

    if(chksb->magicsig != sb->s_magic)
    {
        printk("Not a lightfs filesystem.\n");
        ret = -EIO;
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
        printk(KERN_ERR "Error was detected during fs mount. Check filesystem.\n");
        ret = -EIO;
        goto release;
    }

    sbi->block_size = chksb->block_size;
    sbi->total_block_count = chksb->total_block_count;
    sbi->created_os = chksb->created_os;
    sbi->data_block_num = chksb->data_block_num;
    sbi->inode_block_num = chksb->inode_block_num;
    sbi->mount_time = chksb->mount_time;
    sbi->inode_size = chksb->inode_size;
    sbi->root_inode = chksb->root_inode;
    sbi->last_check_time = chksb->last_check_time;
    sbi->created_os = chksb->created_os;
    sbi->error = 1;
    sb->s_fs_info = sbi;
    brelse(sbh);

    root_inode = lightfs_iget(sb, sbi->root_inode);
    if(IS_ERR(root_inode))
    {
        ret = PTR_ERR(root_inode);
        goto release_sbi;
    }

    sb->s_root = d_make_root(root_inode);
    if(!sb->s_root)
    {
        ret = -ENOMEM;
        goto release_sbi;
    }

    return ret;
release_sbi:
    kfree(sbi);
release:
    brelse(sbh);
    return ret;
}