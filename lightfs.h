#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/bpf.h>
/*
 - Structure of the filesystem

    +---------------+
    | 1024 reserved |
    +---------------+
    | superblock    |
    +---------------+
    | inode bitmap  |
    | (1 and 0)     |
    +---------------+
    | data bitmap   |
    +---------------+
    | inode area    |
    +---------------+
    | data area     |
    +---------------+

    superblock structure is 1024 bytes, and the inode is 256 bytes.
    the filesystem is divided into logical blocks, which is 1024 bytes in size.
    all the structure is designed to fit in within this size. exactly 4 inodes will fit
    inside one logical blocks, and data block size is expected to be n * 1024 bytes in size.
*/

#define lightfs_magic 0x20070207
#define lightfs_fnlen 60
#define LIGHTFS_LOGICAL_BS 1024
#define NULL ((void *)0)
#define NUM_LB_PB(block_size) LIGHTFS_LOGICAL_BS / block_size

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
    void* inode_bmap;
    void* data_bitmap;
    char padding[926];
};

//This is the inode structure for the filesystem. It is in 256 bytes in size.
struct lightfs_inode {
    __u32 i_mode;
    __u32 i_uid;
    __u32 i_gid;
    __u32 i_size;
    struct timespec64 i_atime;
    struct timespec64 i_mtime;
    struct timespec64 i_ctime;
    __u32 blocks;
    __u32 block[12];
    __u32 ind_blk[4];
    __u32 d_ind_blk[2];
    char padding[116];
};

//inode container
struct lightfs_inode_info {
    struct inode vfs_inode;
    __u32 blocks;
    __u32 *block;
    __u32 *ind_blk;
    __u32 *d_ind_blk;
};

//dentry for lightfs
struct lightfs_dentry {
    char filename[lightfs_fnlen];
    __u64 inode;
};

struct lightfs_d_head {
    __u64 magic;
    __u64 item_num;
    char padding[56];
};
int lightfs_get_first_bit(struct super_block *sb);

static int lightfs_fill_super(struct super_block *sb, void *data, int silent);
static void lightfs_put_super(struct super_block *sb);
static struct dentry *lightfs_mount(struct file_system_type *fs_type,
                              int flags,
                              const char *dev_name,
                              void *data);

static int lightfs_statfs(struct dentry *dentry, struct kstatfs *buf);
static int lightfs_syncfs(struct super_block *sb, int wait);
static void lightfs_kill_super(struct super_block *sb);

int lightfs_get_bitmap(struct super_block *sb);
void lightfs_free_bitmap(struct super_block *sb);


struct buffer_head **get_block(struct super_block *sb, __u32 num);

static struct inode *lightfs_iget(struct super_block *sb, size_t inode);
static struct dentry *lightfs_lookup(struct inode *dir,
                            struct dentry *dentry,
                            unsigned int flags);
static int lightfs_create(struct mnt_idmap *id,
                       struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl);

int init_dir(struct super_block *sb, struct inode *dir, struct inode *parent);