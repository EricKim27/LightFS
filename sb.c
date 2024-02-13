#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "lightfs.h"

//getting superblock data from device
int get_sb(const struct lightfs_superblock *superblock, int device){
    off_t start_sb = lseek(device, 1024, SEEK_SET);

    if(start_sb == -1){
        perror("Failed to move to superblock location.");
        return 3;
    }
    if (read(device, superblock, sizeof(struct lightfs_superblock)) == -1) {
        perror("failed to read superblock.");
        return 4;
    }
    return 0;
}

//checking if the filesystem is LightFS
int check_fs(int device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret == NULL){
        perror("read error");
        
        return 2;
    }
    if(superblock->magicsig != 0x10E){
        perror("Invalid filesystem");
        
        return 1;
    }

    printf("disk seems to be using a valid lightfs filesystem. \n");
    
    return 0;
}

//used for fetching number of free data blocks
uint64_t sb_freeblock(int device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret > 0){
        perror("Failed to get superblock.");
        
        return -1;
    }
    uint64_t freedata = superblock->free_data;
    
    return freedata;
}
uint64_t sb_totalblock(int device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret > 0){
        perror("Failed to get superblock.");
        
        return -1;
    }
    uint64_t totaldata = superblock->total_block_count;
    
    return totaldata;
}
uint64_t sb_bs(int device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret > 0){
        perror("Failed to get superblock.");
        
        return -1;
    }
    uint64_t blocksize = superblock->block_size;
    
    return blocksize;

}
uint64_t sb_total_inode(int device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret > 0){
        perror("Failed to get superblock.");
        return -1;
    }
    uint64_t totaldata = superblock->data_block_num;
    return totaldata;
}
//Used for fetching number of free inodes
uint64_t sb_free_inode(int device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret > 0){
        perror("Failed to get superblock.");
        return -1;
    }
    uint64_t freeinode = superblock->free_inode;
    return freeinode;
}
//This function is for writing superblocks to the disk. It is used when formatting a disk to lightfs.
int write_sb(int device, const struct lightfs_superblock *superblock){
    off_t start_sb = lseek(device, 1024, SEEK_SET);
    if(start_sb == -1){
        perror("Failed to move to superblock location.");
        return 2;
    }
    if(write(device, superblock, sizeof(struct lightfs_superblock)) == -1){
        perror("Failed to write superblock");
        return 3;
    }
    return 0;
}