#include "lightfs.h"
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/mpage.h>

//going to implement writepage here.
__u32 physical_to_logical(__u32 physical, struct super_block *sb) 
{
    struct lightfs_superblock *sbi = sb->s_fs_info;
    return physical * (sbi->block_size / LIGHTFS_LOGICAL_BS) - ((sbi->block_size / LIGHTFS_LOGICAL_BS) - 1);
}
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
    if(!buf) {
        return NULL;
    }
    unsigned int i;

    for(i = 0; i<logical_per_physical; i++) {
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
static int lightfs_file_get_block(struct inode *inode,
                                   sector_t iblock,
                                   struct buffer_head *bh_result,
                                   int create)
{
    __u32 blk_num = (__u32)iblock;
    bh_result = alloc_buffer_head(GFP_KERNEL);
    if(!bh_result)
    {
        printk(KERN_ERR "Error while allocating buffer head at get_block\n");
        return -EINVAL;
    }
    memset(bh_result, 0, sizeof(struct buffer_head));
    struct super_block *sb = inode->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    char *res;

    res = get_block(sb, blk_num);
    if(res == NULL) {
        printk(KERN_ERR "Error 1 in file_get_block\n");
        brelse(bh_result);
        return -EIO;
    }
    bh_result->b_blocknr = iblock;
    bh_result->b_bdev = inode->i_sb->s_bdev;
    bh_result->b_size = sbi->block_size;
    bh_result->b_data = res;
    return 0;
}
static int lightfs_get_logical(struct inode *inode,
                               sector_t iblock,
                               struct buffer_head *bh_result,
                               int create)
{
    __u32 b_num = (__u32)iblock;
    struct super_block *sb = inode->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh = NULL;
    size_t ino;
    int ino_shift = (sbi->inode_block_num * sizeof(struct lightfs_inode)) % LIGHTFS_LOGICAL_BS;
    if(ino_shift == 0)
        ino = ((sbi->inode_block_num * sizeof(struct lightfs_inode)) / LIGHTFS_LOGICAL_BS);
    else
        ino = ((sbi->inode_block_num * sizeof(struct lightfs_inode)) / LIGHTFS_LOGICAL_BS) + 1;
    loff_t start_block = 1 + ((sbi->data_block_num / LIGHTFS_LOGICAL_BS) + 1) + ((sbi->inode_block_num / LIGHTFS_LOGICAL_BS) + 1) + ino + b_num;
    bh = sb_bread(sb, start_block);
    if(!bh){
        return -EINVAL;
    }
    bh_result = bh;
    return 0;
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
    if(!bh) {
        return -EIO;
    }
    
    for(i=0; i<(sbi->block_size / LIGHTFS_LOGICAL_BS); i++) {

        memcpy(bh[i]->b_data, buf+(i*1024), LIGHTFS_LOGICAL_BS);
        printk(KERN_INFO "syncing logical block %d with disk\n", i);

        mark_buffer_dirty(bh[i]);
        brelse(bh[i]);
    }
    if(change_bbitmap(sb, block_no) < 0) {
        kfree(buf);
        return -EFAULT;
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

static int lightfs_open(struct inode *inode, struct file *file)
{
    struct lightfs_inode_info *i_info = inode->i_private;
    struct super_block *sb = inode->i_sb;
    bool wronly = (file->f_flags & O_WRONLY);
    bool rdwr = (file->f_flags & O_RDWR);
    bool trunc = (file->f_flags & O_TRUNC);

    if((wronly || rdwr) && trunc && inode->i_size) {
        //TODO: Think of a trunication method
    }
    return 0;
}
static ssize_t lightfs_read(struct file *file, 
                            char __user *buf, 
                            size_t len, 
                            loff_t *ppos)
{
    struct inode *inode = file->f_mapping->host;
    struct lightfs_inode_info *ci = inode->i_private;
    struct address_space *mapping = inode->i_mapping;
    struct super_block *sb = inode->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    ssize_t ret = 0;
    loff_t pos = *ppos;
    loff_t size = i_size_read(inode);
    loff_t start_block = pos / sbi->block_size;
    __u32 **b_num = ci->block;
    char *kbuf = (char *)kmalloc(size, GFP_KERNEL);
    char *block;
    if(b_num[1] == NULL)
        return -ENOENT;
    
    __u32 number_of_blocks;
    loff_t data_shift = size % sbi->block_size;

    if(data_shift == 0) {
        number_of_blocks = size / sbi->block_size;
    } else {
        number_of_blocks = size / sbi->block_size + 1;
    }

    char *dbuf = (char *)kmalloc(number_of_blocks * sbi->block_size, GFP_KERNEL);
    uint i;
    for(i=0; i<number_of_blocks; i++) {
        block = get_block(sb, *b_num[i+start_block]);
        if(block == NULL){
            kfree(kbuf);
            return -EINVAL;
        }
        kfree(block);
    }
    memcpy(kbuf, dbuf + data_shift, size);
    if (data_shift + size > number_of_blocks * sbi->block_size) {
        kfree(kbuf);
        kfree(dbuf);
        return -EINVAL;
    }
    if(copy_to_user(buf, kbuf, size)) {
        kfree(kbuf);
        kfree(dbuf);
        return -EFAULT;
    }
    ret += size;
    len -= size;
    pos += size;
    return ret;
}

static ssize_t lightfs_write(struct file *file,
                              const char __user *buf,
                              size_t len,
                              loff_t *ppos)
{
    struct inode *inode = file->f_inode;
    struct super_block *sb = inode->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    struct lightfs_inode_info *ci = inode->i_private;
    struct buffer_head *bh;
    loff_t pos = *ppos;
    size_t size = i_size_read(inode);
    ssize_t ret = 0;
    __u32 number_of_blocks;
    loff_t start_block = pos / sbi->block_size;
    loff_t block_shift = size % sbi->block_size;

    if (!file || !buf || !ppos)
        return -EINVAL;
    if(block_shift == 0) {
        number_of_blocks = size / sbi->block_size;
    } else {
        number_of_blocks = size / sbi->block_size + 1;
    }

    __u32 **b_num = ci->block;
    char *dat = (char *)kmalloc(size, GFP_KERNEL);
    if(!dat) {
        return -ENOMEM;
    }
    if(copy_from_user(dat, buf, len)) {
        kfree(dat);
        return -EFAULT;
    }
    size_t i;
    __u32 block_size = sbi->block_size;
    char *block;
    for(i = 0; i<number_of_blocks; i++) {
        block = get_block(sb, b_num[i + start_block]);
        memcpy(block, dat + i * sbi->block_size, block_size);
        sync_block(sb, b_num[i+start_block], block);
    }
    ret += size;
    len -= size;
    pos += size;

    *ppos = pos;
    kfree(dat);
    return ret;
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

    for(i = 0; i<logical_per_physical; i++) {
        bh[i] = sb_bread(sb, db_offset+i);

        if(!bh) {
            return NULL;
        }
    }
    return bh;
}

static void lightfs_readahead(struct readahead_control *rac)
{
    return mpage_readahead(rac, lightfs_file_get_block);
}
//TODO: since the physical block size and logical block sizes differ, it is required to think of a page mapping and writing code.
static int lightfs_writepage(struct page *page, struct writeback_control *wbc)
{
    struct inode *ino = page->mapping->host;
    struct lightfs_inode_info *ci = ino->i_private;
    struct super_block *sb = ino->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    char *block;
    loff_t offset = page_offset(page);
    sector_t start_in_block = offset / sbi->block_size;
    sector_t number_of_blocks;
    sector_t block_shift = offset % sbi->block_size;
    int err;

    if(block_shift == 0)
        number_of_blocks = PAGE_SIZE / sbi->block_size;
    else
        number_of_blocks = PAGE_SIZE / sbi->block_size + 1;

    __u32 block_list[number_of_blocks];
    sector_t number_of_logical;
    int i;
    lock_page(page);
    char *page_data = kmap(page);
    for(i=0; i<number_of_blocks; i++){
        block_list[i] = ci->block[start_in_block + i];
    }

    block = get_block(sb, block_list[0]);
    memcpy(block + block_shift, page_data, PAGE_SIZE - block_shift);
    sync_block(sb, block_list[0], block);
    if(number_of_blocks == 2) {
        block = get_block(sb, block_list[1]);
        memcpy(block, page_data + (PAGE_SIZE - block_shift), block_shift);
        sync_block(sb, block_list[1], block);
    }

    kunmap(page);
    unlock_page(page);
    err = 0;

    lock_page(page);


    unlock_page(page);
    return err;
}

//Since most block I/O prep is done on lightfs_write, this code is mainly for checking permission and filesize.
static int lightfs_write_begin(struct file *file,
                                struct address_space *mapping,
                                loff_t pos,
                                unsigned int len,
                                struct page **pagep,
                                void **fsdata)
{
    if(len > LIGHTFS_MAX_FSIZE) {
        return -ENOSPC;
    }
     if (!may_write_to_inode(file->f_inode)) {
        return -EACCES;
    }

    return 0;
}


const struct file_operations lightfs_file_operations = {
    .owner = THIS_MODULE,
    .open = &lightfs_open,
    .read = lightfs_read,
    .write = lightfs_write,
    .llseek = generic_file_llseek,
    .fsync = generic_file_fsync,
};

const struct address_space_operations lightfs_addr_ops = {
    .readahead = lightfs_readahead,
    .write_begin = lightfs_write_begin,
    .write_end = generic_write_end,
};