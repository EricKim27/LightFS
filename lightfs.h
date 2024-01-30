#include <stdint.h>
#include <sys/types.h>

//Superblock Structure
struct superblock {
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
struct lightfs_inode {
    uint32_t imode;
    uint32_t uid;
    uint32_t gid;
    
};
struct dirblk_entry {
    uint64_t inode;
    char name[128];
};