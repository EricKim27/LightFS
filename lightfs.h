#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/bpf.h>
/*
 - Structure of the filesystem:

    +---------------+
    | 1024 reserved |
    +---------------+
    |  superblock   |
    +---------------+
    |  data bitmap  |
    |   (1 and 0)   |
    +---------------+
    |  inode bitmap |
    +---------------+
    |   inode area  |
    +---------------+
    |   data area   |
    +---------------+

    superblock structure is 1024 bytes, and the inode is 256 bytes.
*/
#define lightfs_magic 0x20070207
#define lightfs_fnlen 60
#define LIGHTFS_LOGICAL_BS 1024
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
    bool *inode_bmap;
    bool *data_bitmap;
    char padding[926];
};

struct lightfs_inode {
    __u32 i_mode;
    __u32 i_uid;
    __u32 i_gid;
    __u32 i_size;
    __u32 i_atime;
    __u32 i_mtime;
    __u32 i_ctime;
    __u32 blocks;
    __u32 block[12];
    __u32 ind_blk[4];
    __u32 d_ind_blk[2];
    char padding[152];
};

int lightfs_fill_super(struct super_block *sb, void *data, int silent);
void lightfs_put_super(struct super_block *sb);
int lightfs_get_bitmap(struct super_block *sb);
void lightfs_free_bitmap(struct super_block *sb);
int lightfs_statfs(struct dentry *dentry, struct kstatfs *buf);
int lightfs_syncfs(struct super_block *sb, int wait);
void lightfs_kill_super(struct super_block *sb);

struct dentry *lightfs_mount(struct file_system_type *fs_type,
                              int flags,
                              const char *dev_name,
                              void *data);
struct inode *lightfs_iget(struct super_block *sb, __u64 inode);