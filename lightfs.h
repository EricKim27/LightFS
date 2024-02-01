#include <stdint.h>
#include <sys/types.h>

//Superblock Structure
struct lightfs_superblock {
    uint64_t inode_block_num;
    uint64_t total_block_count;
    uint64_t data_block_num;
    uint64_t free_inode;
    uint64_t free_data;
    uint32_t first_data;
    uint32_t block_size;
    uint32_t data_grp_size;
    uint32_t inode_grp_size;
    uint32_t inode_size;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t magicsig;
    uint16_t state;
};
//inode structure
struct lightfs_inode {
    uint64_t inum;
    uint32_t imode;
    uint32_t access_time;
    uint32_t modified_time;
    uint32_t created_time;
    uint32_t delete_time;
    uint64_t direct_block_ptr[12];
    uint64_t indirect_block_ptr[6];
    uint64_t indirect_block_double[3];
    uint16_t number_of_inodes;
    uint16_t uid;
    uint16_t gid;
};
//dir entry
struct d_entry {
    uint64_t inode;
    char name[128];
};

extern int lightfs_readdir(char *name[128]);