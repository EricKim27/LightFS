#include <stdint.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <linux/types.h>

//Superblock Structure
struct lightfs_superblock {
    uint64_t inode_block_num;
    uint64_t total_block_count;
    uint64_t data_block_num;
    uint64_t free_inode;
    uint64_t free_data;
    uint32_t first_data;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t mount_time;
    uint32_t last_check_time;
    uint32_t created_os;
    uint32_t write_time;
    uint16_t magicsig;
    uint16_t state;
    uint16_t error;
};
//inode structure
struct lightfs_inode {
    uint ftype;
    uint root_inode;
    uint64_t inum;
    uint32_t imode;
    uint32_t access_time;
    uint32_t modified_time;
    uint32_t created_time;
    uint32_t delete_time;
    uint64_t direct_block_ptr[24];
    uint64_t indirect_block_ptr[12];
    uint64_t indirect_block_double[6];
    uint64_t indirect_block_triple[3];
    uint16_t number_of_inodes;
    uint64_t next_inode_ptr;
    uint16_t uid;
    uint16_t gid;
};
//dir entry
struct d_entry {
    uint64_t inode;
    char name[128];
};
struct d_header{
    uint64_t total_item_num;
};

struct file_system_type lightfs_type{
    .name = "lightfs",
    .mount = lightfs_mount,
    .kill_sb = lightfs_kill_sb,
    .fs_flags = FS_USERNS_MOUNT,
};
//inode functions
extern uint64_t allocate_inode(int device);
extern int read_inode(struct lightfs_inode *inode, int device);
extern int write_inode(const struct lightfs_inode *inode, int device);
extern uint64_t sb_total_inode(int device);
extern uint64_t sb_free_inode(int device);

//superblock functions
extern int get_sb(const struct lightfs_superblock *superblock, int device);
extern int check_fs(int device);
extern uint64_t sb_freeblock(int device);
extern uint64_t sb_free_inode(int device);
extern int write_sb(int device, const struct lightfs_superblock *superblock);

//directory functions
extern int lightfs_readdir(const struct d_entry *d_entry, int device);
extern int lightfs_writedir(const struct d_entry *d_entry, int device, uint64_t inode);

//file functions
extern int lightfs_writefile(const char name[128]);
extern int lightfs_readfile(const char name[128]);
