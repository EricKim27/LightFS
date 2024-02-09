#include "lightfs.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

uint64_t allocate_inode(const char *device){
    int device_f = open(device, O_RDWR);
    if(device_f < 0){
        perror("File open failed.");
        return -1;
    }
    uint64_t number_of_inodes = sb_total_inode(device);
    off_t start_bitmap = lseek(device_f, 1024 + sizeof(struct lightfs_superblock), SEEK_SET);
    if(start_bitmap == -1){
        perror("Failed to move to bitmap location.");
        close(device_f);
        return -1;
    }
    for(int i=0; i<=number_of_inodes; i++){
        char bitmap;
        if (read(device_f, &bitmap, 1) == -1) {
            perror("failed to read bitmap.");
            close(device_f);
            return -1;
        }
        if(bitmap == 0) {
            close(device_f);
            return i;
        }
        off_t next_bitmap = lseek(device_f, 1, SEEK_CUR);
    }
    printf("No free inodes available.\n");
    close(device_f);
    return -1;
}
int read_inode(struct lightfs_inode *inode, const char *device){
    int device_f = open(device, O_RDONLY);
    if(device_f < 0){
        perror("File open failed.");
        return -1;
    }
    uint64_t number_of_inodes = sb_total_inode(device);
    off_t start_inode = lseek(device_f, 1024 + sizeof(struct lightfs_superblock) + (number_of_inodes / 8) + (sizeof(struct lightfs_inode) * inode->inum), SEEK_SET);
    if(start_inode == -1){
        perror("Failed to move to inode location.");
        close(device_f);
        return -1;
    }
    if (read(device_f, inode, sizeof(struct lightfs_inode)) == -1) {
        perror("failed to read inode.");
        close(device_f);
        return -1;
    }
    close(device_f);
    return 0;
}