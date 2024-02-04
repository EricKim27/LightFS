#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "lightfs.h"

//getting superblock data from device
int get_sb(const char *device){
    struct lightfs_superblock *superblock = malloc(sizeof(struct lightfs_superblock));
    if (superblock == NULL) {
        perror("Memory allocation failed.");
        return NULL;
    }

    int device_f = open(device, O_RDONLY);

    if(device_f < 0){
        perror("File open failed.");
        free(superblock);
        return NULL;
    }
    off_t start_sb = lseek(device_f, 1024, SEEK_SET);

    if(start_sb == -1){
        perror("Failed to move to superblock location.");
        free(superblock);
        close(device_f);
        return NULL;
    }
    if (read(device_f, superblock, sizeof(struct lightfs_superblock)) == -1) {
        perror("failed to read superblock.");
        free(superblock);
        close(device_f);
        return NULL;
    }

    close(device_f);
    return superblock;
}

//checking if the filesystem is LightFS
int check_fs(const char *device){
    struct lightfs_superblock *superblock = get_sb(device);
    if(superblock == NULL){
        perror("read error");
        free(superblock);
        return 2;
    }
    if(superblock->magicsig != 0x10E){
        perror("Invalid filesystem");
        free(superblock);
        return 1;
    }

    printf("disk seems to be using a valid lightfs filesystem. \n");
    free(superblock);
    return 0;
}

//used for fetching number of free data blocks
int sb_freeblock(const char *device){
    struct lightfs_superblock *superblock = get_sb(device);
    uint64_t freedata = superblock->free_data;
    free(superblock);
    return freedata;
}
//Used for fetching number of free inodes
int sb_free_inode(const char *device){
    struct lightfs_superblock *superblock = get_sb(device);
    uint64_t freeinode = superblock->free_inode;
    free(superblock);
    return freeinode;
}
//This function is for writing superblocks to the disk. It is used when formatting a disk to lightfs.
int write_sb(const char *device, const struct lightfs_superblock *superblock){
    int device_f = open(device, O_WRONLY);
    if(device_f < 0){
        perror("File open failed.");
        free(superblock);
        return 1;
    }
    off_t start_sb = lseek(device_f, 1024, SEEK_SET);
    if(start_sb == -1){
        perror("Failed to move to superblock location.");
        free(superblock);
        close(device_f);
        return 2;
    }
    if(write(device_f, superblock, sizeof(struct lightfs_superblock)) == -1){
        perror("Failed to write superblock");
        free(superblock);
        close(device_f);
        return 3;
    }
    free(superblock);
    close(device_f);
    return 0;
}