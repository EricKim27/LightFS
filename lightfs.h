#define lightfs_magic 0x20070207
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

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
