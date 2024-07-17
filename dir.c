#include "lightfs.h"
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/string.h>

//iterates through dir blocks
int lightfs_iterate_dir(struct inode *dir, struct dentry *dentry)
{
    struct super_block *sb = dir->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct lightfs_dentry *p_dentry = NULL;
    struct lightfs_d_head *head = NULL;
    struct buffer_head **bh = NULL;
    struct lightfs_inode_info *i_info = dir->i_private;
    unsigned int lb, pb;
    
    bh = get_block(sb, i_info->block[0]);
    if(!bh)
    {
        printk(KERN_ERR "Error on dir.c line 17");
        return -EIO;
    }
    head = (struct lightfs_d_head *)bh[0]->b_data;
    for(pb = 0; pb < head->item_num; pb++)
    {
        bh = get_block(sb, i_info->block[pb]);
        if(pb == 0)
            bh[0] = (struct buffer_head *)((char *)bh[0] + sizeof(struct lightfs_d_head));
        
        for(lb = 0; lb < 16; lb++)
        {
            p_dentry = (struct lightfs_dentry *)bh[lb];
            if(strncmp(dentry->d_name.name, p_dentry->filename, sizeof(p_dentry->filename)) == 0){
                return lb+pb;
            }
        }
    }
    
    return -ENOENT;
}

