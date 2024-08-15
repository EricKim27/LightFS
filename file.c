#include "lightfs.h"
#include <linux/buffer_head.h>
#include <linux/slab.h>

/* plans to change this function: return a pointer to buffer containing data
 * and then later use sync_block to sync the data to the disk.
 */
//going to implement readpage and writepage here.
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
    if(get_block == NULL) {
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
    char *kbuf;
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
    char *block = (char *)kmalloc(sbi->block_size, GFP_KERNEL);
    for(i = 0; i<number_of_blocks; i++) {
        block = get_block(sb, ci->block[i + start_block]);
        memcpy(block, dat + i * sbi->block_size, block_size);
        sync_block(sb, ci->block[i+start_block], block);
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
int lightfs_readpage(struct file *file, struct page *page) {
    struct inode *inode = file->f_mapping->host;
    struct lightfs_inode_info *ci = inode->i_private;
    struct super_block *sb = inode->i_sb;
    struct lightfs_superblock *sbi = sb->s_fs_info;
    char *blk_dat;
    char *file_dat;
    char *kaddr;

    loff_t offset = page_offset(page);
    size_t bytes_to_read = min_t(size_t, PAGE_SIZE, i_size_read(inode) - offset);
    size_t page_location = offset % sbi->block_size;
    __u32 block_start = offset / 4096;
    uint num_blk_to_read;
    if(bytes_to_read % sbi->block_size == 0)
        num_blk_to_read = 1;
    else
        num_blk_to_read = 2;    
    __u32 **blk_num = ci->block;
    file_dat = (char *)kmalloc(sbi->block_size * num_blk_to_read, GFP_KERNEL);

    for(int i=0; i<num_blk_to_read; i++){
        blk_dat = get_block(sb, *blk_num[i]);
        memcpy(file_dat + i*sbi->block_size, blk_dat, sbi->block_size);
    }
    kaddr = kmap(page);
    memcpy(kaddr, file_dat + (bytes_to_read & sbi->block_size), PAGE_SIZE);

    kunmap(page);
    SetPageUptodate(page);
    unlock_page(page);
    kfree(file_dat);
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
    
};