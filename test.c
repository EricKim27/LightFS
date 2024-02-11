#include "lightfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Usage: %s <device>\n", argv[0]);
        return 1;
    }
    struct lightfs_superblock sb = {
        .inode_block_num = 0,
        .total_block_count = 0,
        .data_block_num = 0,
        .free_inode = 0,
        .free_data = 0,
        .first_data = 0,
        .block_size = 0,
        .inode_size = 0,
        .mount_time = 0,
        .last_check_time = 0,
        .created_os = 0,
        .write_time = 0,
        .magicsig = 0x10E,
        .state = 0,
        .error = 0
    };
    int device = open(argv[1], O_RDWR);
    int ret = write_sb(device, &sb);
    if(ret > 0){
        printf("Failed to write superblock.\n");
        return 1;
    }
    printf("Superblock written successfully.\n");
    return 0;
}