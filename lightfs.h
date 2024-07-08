#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/bpf.h>

#define lightfs_magic 0x20070207
#define NULL ((void *)0)

//Superblock Structure(1024 bytes in size)
struct lightfs_superblock {
    __u64 inode_block_num;
    __u64 total_block_count;
    __u64 data_block_num;
    __u64 free_inode;
    __u64 free_data;
    __u64 root_inode;
    __u32 block_size;
    __u32 inode_size;
    __u32 mount_time;
    __u32 last_check_time;
    __u32 created_os;
    __u32 write_time;
    __u16 magicsig;
    __u16 state;
    __u16 error;
    char padding[942];
};

int lightfs_fill_super(struct super_block *sb, void *data, int silent);
int calculate_root_offset(int num);
void lightfs_put_super(struct super_block *sb);
int lightfs_statfs(struct dentry *dentry, struct kstatfs *buf);
int lightfs_syncfs(struct super_block *sb, int wait);

struct inode *lightfs_iget(struct super_block *sb, __u64 inode);