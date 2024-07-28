#include "lightfs.h"
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/dcache.h>
#include <linux/string.h>

//TODO: need to fix all the buffer head variable to fit the newly revised get_block() function.

static const struct inode_operations lightfs_inode_operations;
//getting inode structure from disk
struct inode *lightfs_iget(struct super_block *sb, size_t inode)
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh = NULL;
    struct lightfs_inode *raw_inode = NULL;
    struct lightfs_inode_info *ci = NULL;
    struct inode *mem_inode = iget_locked(sb, inode);
    ci = kmalloc(sizeof(struct lightfs_inode_info), GFP_KERNEL);
    mem_inode->i_ino = inode;
    mem_inode->i_sb = sb;
    mem_inode->i_private = ci;
    //TODO: Verify if the calculated offset is correct
    __u32 inode_offset = 1 + ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + ((sbi->inode_block_num / LIGHTFS_LOGICAL_BS) + 1) + (inode / LIGHTFS_LOGICAL_BS);
    unsigned int inode_location_inlb = inode % LIGHTFS_LOGICAL_BS;
    bh = sb_bread(sb, inode_offset);

    if(!bh)
    {
        printk(KERN_ERR "Error while reading inode,\n");
        goto error;
    }

    //fill inode
    raw_inode = (struct lightfs_inode *)(bh->b_data + (inode_location_inlb * sizeof(struct lightfs_inode)));
    i_gid_write(mem_inode, raw_inode->i_gid);
    i_uid_write(mem_inode, raw_inode->i_uid);
    mem_inode->i_size = le32_to_cpu(raw_inode->i_size);
    mem_inode->i_mode = le32_to_cpu(raw_inode->i_mode);
    mem_inode->__i_atime = raw_inode->i_atime;
    mem_inode->__i_mtime = raw_inode->i_mtime;
    mem_inode->__i_ctime = raw_inode->i_ctime;
    __u32 **blk = (__u32 **)get_block(sb, raw_inode->block_no_blk);
    ci->block = blk;

     if (S_ISDIR(mem_inode->i_mode)) {
        //TODO: define operations
        mem_inode->i_fop = &lightfs_dir_operations;
     } else if(S_ISREG(mem_inode->i_mode)){
        //TODO: define operations
        mem_inode->i_fop = &lightfs_file_operations;
     } else if(S_ISLNK(mem_inode->i_mode)){
        //TODO: define operations
        //mem_inode->i_fop = &lightfs_link_operations;

     }
     mem_inode->i_op = &lightfs_inode_operations;
    brelse(bh);
    unlock_new_inode(mem_inode);

    return mem_inode;
error:
    brelse(bh);
    return NULL;
}

//TODO: Needs to be corrected accordingly to the changed inode structure
static struct dentry *lightfs_lookup(struct inode *dir,
                            struct dentry *dentry,
                            unsigned int flags)
{
    struct dentry *found_dentry;
    struct super_block *sb = dir->i_sb;
    struct inode *i_found_dentry;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct lightfs_inode_info *ci = dir->i_private;
    struct lightfs_dentry *dentry_info = NULL;

    found_dentry = d_lookup(dentry, &dentry->d_name);
    if(found_dentry) {
        return found_dentry;
    }
    
    unsigned int i;
    unsigned int j;
    void *raw_dir = kmalloc(ci->blocks * sbi->block_size, GFP_KERNEL);
    if(!raw_dir)
    {
        printk(KERN_ERR "Error while allocating area for dentry_lookup: line 89 @ inode.c\n");
        kfree(raw_dir);
        return NULL;
    }
    //directory block has the maximum block of 12 blocks.
    char *dir_blk = (char *)kmalloc(ci->blocks * sbi->block_size, GFP_KERNEL);
    if(dir_blk == NULL)
    {
        printk(KERN_ERR "Error while allocating area for directory: line 94 @ inode.c\n");
        return NULL;
    }
    char *buf;
    __u32 **arr = ci->block;
    for(i=0; i<ci->blocks && i<12; i++)
    {   
        buf = get_block(sb, *arr[i]);
        if(buf == NULL)
        {
            printk(KERN_ERR "Error while get_block() function: line 107 @ inode.c\n");
        }
        memcpy(dir_blk + (i * sbi->block_size), buf, sbi->block_size);
        kfree(buf);
    }

    dentry_info = (struct lightfs_dentry *)(raw_dir + sizeof(struct lightfs_d_head));
    size_t num_entries = (ci->blocks * sbi->block_size) / sizeof(struct lightfs_dentry);
    for(i=0; i<num_entries; i++)
    {
        if(strncmp(dentry_info->filename, dentry->d_name.name, sizeof(dentry_info->filename)))
        {
            //TODO: return dentry job
            i_found_dentry = lightfs_iget(sb, dentry_info->inode); //get the found dentry's inode
            d_instantiate(found_dentry, i_found_dentry); //assosiate dentry with its inode
            kfree(raw_dir);
            return found_dentry;
        }
        dentry_info++;
    }
    kfree(raw_dir);
    return NULL;
}

static int lightfs_create(struct mnt_idmap *id,
                       struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
    struct lightfs_dentry *dentry_location = NULL;
    struct lightfs_dentry *dentry_from = kmalloc(sizeof(struct lightfs_dentry), GFP_KERNEL);
    struct super_block *sb = dir->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct inode *inode;
    struct lightfs_inode *inode_i;
    struct lightfs_inode_info *ii;
    struct buffer_head *ibh; //buffer head for inode
    struct buffer_head *bbh; //buffer head for bitmap
    struct lightfs_d_head *dh;
    inode = new_inode(sb);
    unsigned int i;

    inode->i_ino = get_next_ino();
    inode_init_owner(id, inode, dir, mode);
    inode->i_size = 0;
    inode->__i_atime = inode->__i_mtime = inode->__i_ctime = current_time(inode);
    
    size_t b_offset = 1 + (inode->i_ino / LIGHTFS_LOGICAL_BS) + 1; //This calculation method needs to change.
    size_t b_shift = inode->i_ino % LIGHTFS_LOGICAL_BS;
    bbh = sb_bread(sb, b_offset);
    bool *bmap_mark = (bool *)(bbh->b_data) + b_shift;
    *bmap_mark = 1;

    size_t i_offset = 1 + ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + ((sbi->inode_block_num / LIGHTFS_LOGICAL_BS) + 1) + (inode->i_ino / 4) + 1;
    size_t i_shift = inode->i_ino % 4;
    ibh = sb_bread(sb, i_offset);
    inode_i = (struct lightfs_inode *)(ibh->b_data) + i_shift;

    //TODO: set operations
    
    inode_i->i_atime = inode_i->i_mtime = inode_i->i_ctime = inode->__i_atime;
    inode_i->i_mode = inode->i_mode;
    inode_i->i_gid = inode->i_gid.val;
    inode_i->i_uid = inode->i_uid.val;
    inode_i->i_size = inode->i_size;
    //the bottom three line will be changed due to changes in design of storing data block numbers.
    inode_i->block_no_blk = lightfs_get_first_bit(sb);
    d_instantiate(dentry, inode);
    //add entry to block
    strncpy(dentry_from->filename, dentry->d_name.name, sizeof(dentry_from->filename) - 1);
    dentry_from->filename[sizeof(dentry_from->filename) - 1] = '\0';
    dentry_from->inode = dentry->d_inode->i_ino;
    char *buf;
    buf = get_block(sb, *(ii->block[0]));
    dh = (struct lightfs_d_head *) buf;

    size_t block_num = dh->item_num+1 / 64; //What was this for?
    size_t block_shift = dh->item_num+1 % 64;
    if(S_ISDIR(inode->i_mode)) {
        int ret = init_dir(sb, inode, dir);
        if(ret != 0)
            printk(KERN_ERR "Error at line 190 @ inode.c\n");
    }
    if(block_shift == 0) {
        buf = get_block(sb, *(ii->block)[block_num-1]);

    } else {
        buf = get_block(sb, *(ii->block)[block_num]);
    }
    if(buf == NULL) {
        printk(KERN_ERR "Error at line 199 @ inode.c\n");
        return -EIO;
    }
    //TODO: change the function to fit the revised get_block
    size_t dentry_tail = (dh->item_num+1) - 64 * (block_num - 1);
    dentry_location = ((struct lightfs_dentry *)buf + dh->item_num + 2);
    
    memcpy(dentry_location, dentry_from, sizeof(struct lightfs_dentry));

    mark_buffer_dirty(ibh);
    mark_buffer_dirty(bbh);
    mark_inode_dirty(inode);
    brelse(ibh);//free the inode buffer head
    brelse(bbh);//free the bitmap buffer head
    sync_block(sb, *(ii->block[0]), buf);
    kfree(dentry_from);
    return 0;
}
static int lightfs_mkdir(struct mnt_idmap *id, struct inode *dir, struct dentry *dentry, umode_t mode)
{
    return lightfs_create(id, dir, dentry, mode | S_IFDIR, 0);
}

static const struct inode_operations lightfs_inode_operations = {
    .lookup = lightfs_lookup,
    .create = lightfs_create,
    .mkdir = lightfs_mkdir,
};