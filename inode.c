#include "lightfs.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

uint64_t allocate_inode(int device){
    uint64_t number_of_inodes = sb_total_inode(device);
    off_t start_bitmap = lseek(device, 1024 + sizeof(struct lightfs_superblock), SEEK_SET);
    if(start_bitmap == -1){
        perror("Failed to move to bitmap location.");
        close(device);
        return -1;
    }
    for(int i=0; i<=number_of_inodes; i++){
        char bitmap;
        if (read(device, &bitmap, 1) == -1) {
            perror("failed to read bitmap.");
            return -1;
        }
        if(bitmap == 0) {
            return i;
        }
        off_t next_bitmap = lseek(device, 1, SEEK_CUR);
    }
    printf("No free inodes available.\n");
    return -1;
}

int read_inode(struct lightfs_inode *inode, int device){
    uint64_t number_of_inodes = sb_total_inode(device);
    off_t start_inode = lseek(device, 1024 + sizeof(struct lightfs_superblock) + (number_of_inodes / 8) + (sizeof(struct lightfs_inode) * inode->inum), SEEK_SET);
    if(start_inode == -1){
        perror("Failed to move to inode location.");
        return -1;
    }
    if (read(device, inode, sizeof(struct lightfs_inode)) == -1) {
        perror("failed to read inode.");
        return -1;
    }
    return 0;
}

int write_inode(const struct lightfs_inode *inode, int device){

    uint64_t number_of_inodes = sb_total_inode(device);
    off_t start_inode = lseek(device, 1024 + sizeof(struct lightfs_superblock) + (number_of_inodes / 8) + (sizeof(struct lightfs_inode) * inode->inum), SEEK_SET);
    if(start_inode == -1){
        perror("Failed to move to inode location.");
        return -1;
    }
    if (write(device, inode, sizeof(struct lightfs_inode)) == -1) {
        perror("failed to write inode.");
        return -1;
    }
    return 0;
}