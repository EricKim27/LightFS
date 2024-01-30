#include <stdint.h>

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
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t magicsig;
    uint16_t state;
};
struct inode_entry {

};
struct directory_entry {

};