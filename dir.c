#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/string.h>

//iterates through dir blocks

int init_dir(struct super_block *sb, struct inode *dir, struct inode *parent)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct lightfs_dentry *dentry;
    struct lightfs_inode_info *i_info = dir->i_private;
    char *my_name = ".";
    char *parent_name = "..";
    char *buf;
    __u32 blk_num = i_info->block[0];
    buf = get_block(sb, blk_num);
    if(buf == NULL)
    {
        printk(KERN_ERR "error at line 49 @ dir.c\n");
        return -EIO;
    }
    dentry = (struct lightfs_dentry *)(buf);
    strncpy(dentry->filename, my_name, strlen(my_name) + 1);
    dentry->inode = dir->i_ino;
    dentry++;

    strncpy(dentry->filename, parent_name, strlen(parent_name) + 1);
    dentry->inode = parent->i_ino;

    sync_block(sb, i_info->block[0], buf);
    return 0;
}