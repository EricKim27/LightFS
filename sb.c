#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "lightfs.h"

//getting superblock data from device
int get_sb(const struct lightfs_superblock *superblock, const char *device){
    if (superblock == NULL) {
        perror("Memory allocation failed.");
        return 1;
    }

    int device_f = open(device, O_RDONLY);

    if(device_f < 0){
        perror("File open failed.");
        return 2;
    }
    off_t start_sb = lseek(device_f, 1024, SEEK_SET);

    if(start_sb == -1){
        perror("Failed to move to superblock location.");
        close(device_f);
        return 3;
    }
    if (read(device_f, superblock, sizeof(struct lightfs_superblock)) == -1) {
        perror("failed to read superblock.");
        close(device_f);
        return 4;
    }
    close(device_f);
    return 0;
}

//checking if the filesystem is LightFS
int check_fs(const char *device){
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
uint64_t sb_freeblock(const char *device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret == NULL){
        perror("Failed to get superblock.");
        
        return -1;
    }
    uint64_t freedata = superblock->free_data;
    
    return freedata;
}
uint64_t sb_total_inode(const char *device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret == NULL){
        perror("Failed to get superblock.");
        return -1;
    }
    uint64_t totaldata = superblock->data_block_num;
    return totaldata;
}
//Used for fetching number of free inodes
uint64_t sb_free_inode(const char *device){
    struct lightfs_superblock *superblock;
    int ret = get_sb(superblock, device);
    if(ret == NULL){
        perror("Failed to get superblock.");
        return -1;
    }
    uint64_t freeinode = superblock->free_inode;
    return freeinode;
}
//This function is for writing superblocks to the disk. It is used when formatting a disk to lightfs.
int write_sb(const char *device, const struct lightfs_superblock *superblock){
    int device_f = open(device, O_WRONLY);
    if(device_f < 0){
        perror("File open failed.");
        return 1;
    }
    off_t start_sb = lseek(device_f, 1024, SEEK_SET);
    if(start_sb == -1){
        perror("Failed to move to superblock location.");
        close(device_f);
        return 2;
    }
    if(write(device_f, superblock, sizeof(struct lightfs_superblock)) == -1){
        perror("Failed to write superblock");
        close(device_f);
        return 3;
    }
    close(device_f);
    return 0;
}