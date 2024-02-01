#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "lightfs.h"

int read_sb(const char *device, struct lightfs_superblock *superblock){
    int device_f = open(device, O_RDONLY);

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
    if (read(device_f, superblock, sizeof(struct lightfs_superblock)) == -1) {
        perror("failed to read superblock.");
        close(device_f);
        return 3;
    }

    close(device_f);
    return superblock;
}
int sb_freeblock(struct lightfs_superblock *){
    //more coming soon
}