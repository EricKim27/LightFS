#include <linux/fs.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/statfs.h>
#include "lightfs.h"

const struct super_operations lightfs_s_ops = {
    .put_super = lightfs_put_super,
    .statfs = lightfs_statfs,
    .sync_fs = lightfs_syncfs,
};
struct dentry *lightfs_mount(struct file_system_type *fs_type,
                              int flags,
                              const char *dev_name,
                              void *data)
{
    struct dentry *dntry = mount_bdev(fs_type, flags, dev_name, data, lightfs_fill_super);
    if(IS_ERR(dntry))
        pr_err("mount failure at %s\n", dev_name);
    else
        pr_info("mount success: %s\n", dev_name);

    return dntry;
}
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
    sbi = kmalloc(sizeof(struct lightfs_superblock), GFP_KERNEL);

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
int lightfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    struct super_block *sb = dentry->d_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;

    buf->f_type = lightfs_magic;
    buf->f_bsize = sbi->block_size;
    buf->f_blocks = sbi->data_block_num;
    buf->f_bfree = sbi->free_data;
    buf->f_bavail = sbi->free_data;
    buf->f_files = sbi->inode_block_num;
    buf->f_ffree = sbi->free_inode;
    buf->f_namelen = lightfs_fnlen;

    return 0;
}
void lightfs_kill_super(struct super_block *sb)
{
    kill_block_super(sb);
    //TODO: additional jobs
    pr_info("unmounted lightfs drive\n");
}

static struct file_system_type lightfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "lightfs",
    .mount = lightfs_mount,
    .kill_sb = lightfs_kill_super,
    .fs_flags = FS_REQUIRES_DEV,
    .next = NULL,
};

static int __init lightfs_initfs(void)
{
    int ret;
    ret = register_filesystem(&lightfs_fs_type);
    if(ret) {
        pr_err("Failed FS registration.\n");
        goto err;
    }
    pr_info("loaded LightFS module\n");
    return 0;
err:
    return ret;
}

static void __exit lightfs_exit(void)
{
    int ret = unregister_filesystem(&lightfs_fs_type);
    if (ret)
        pr_err("Failed to unregister file system\n");
    
    pr_info("unloaded module\n");
}

module_init(lightfs_initfs);
module_exit(lightfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Eric Kim");
MODULE_DESCRIPTION("LightFS filesystem module");