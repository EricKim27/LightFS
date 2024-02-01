#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "lightfs.h"

int get_sb(const char *device){
    struct lightfs_superblock *superblock = malloc(sizeof(struct lightfs_superblock));

    int device_f = open(device, O_RDONLY);

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
    if (read(device_f, superblock, sizeof(struct lightfs_superblock)) == -1) {
        perror("failed to read superblock.");
        free(superblock);
        close(device_f);
        return 3;
    }

    free(superblock);
    close(device_f);
    return superblock;
}

int check_fs(const char *device){
    struct lightfs_superblock *superblock = get_sb(device);
    if(superblock->magicsig != 0x10E){
        perror("Invalid filesystem");
        return 1;
    }
    free(superblock);
    return 0;
}

int sb_freeblock(const char *device){
    struct lightfs_superblock *superblock = get_sb(device);
    return superblock->free_data;
}
int sb_free_inode(const char *device){
    struct lightfs_superblock *superblock = get_sb(device);
    return superblock->free_inode;
}