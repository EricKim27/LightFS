#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>

#define FILL_FS_BYTES 0x22

struct lightfs_superblock {
    uint64_t inode_block_num;
    uint64_t total_block_count;
    uint64_t data_block_num;
    uint64_t free_inode;
    uint64_t free_data;
    uint64_t root_inode;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t mount_time;
    uint32_t last_check_time;
    uint32_t created_os;
    uint32_t write_time;
    uint16_t magicsig;
    uint16_t state;
    uint16_t error;
    void* inode_bmap;
    void* data_bitmap;
    char padding[930];
};

struct lightfs_inode {
    uint32_t i_mode;
    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_size;
    struct timespec i_atime;
    struct timespec i_mtime;
    struct timespec i_ctime;
    uint32_t blocks;
    uint32_t block[12];
    uint32_t ind_blk[4];
    uint32_t d_ind_blk[2];
    char padding[116];
};

struct lightfs_dentry {
    char filename[60];
    uint64_t inode;
};
struct lightfs_d_head {
    uint64_t magic;
    uint64_t item_num;
    char padding[48];
};

//TODO: make a block and inode number calculation mechanism