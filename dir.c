#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/string.h>

int init_dir(struct super_block *sb, struct inode *dir, struct inode *parent)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct lightfs_dentry *dentry;
    struct lightfs_inode_info *i_info = dir->i_private;
    char *my_name = ".";
    char *parent_name = "..";
    char *buf;
    __u32 blk_num = *i_info->block[0];
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

    sync_block(sb, *(i_info->block)[0], buf);
    return 0;
}
static int lightfs_iterate(struct file *dir, struct dir_context *ctx)
{
    int ret = 0;

    return ret;
}

//going to be used for looking for the first empty dentry, as if it's removed, it's going to leave a gap.
int find_first_empty_dentry(struct inode *dir)
{
    struct super_block *sb = dir->i_sb;
    struct lightfs_inode_info *ci = dir->i_private;
    struct lightfs_dentry *d_found;
    char *dir_block = get_block(sb, ci->block[0]);
    if(dir_block == NULL) {
        return -EINVAL;
    }
    int i;
    for(i=0; i<ci->blocks; i++){
        d_found = (struct lightfs_dentry *)(dir_block + i * sizeof(struct lightfs_dentry));
        if(d_found->filename == NULL && d_found->inode == NULL) {
            return i;
        }
    }
    return -ENOENT;
}
const struct file_operations lightfs_dir_operations = {
    .owner = THIS_MODULE,
    .iterate_shared = lightfs_iterate,
};