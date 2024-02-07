#include "lightfs.h"
#include <fcntl.h>

int allocate_inode(const char *device){
    
}
int read_inode(struct lightfs_inode *inode, const char *device){
    int device_f = open(device, O_RDONLY);
    if(device_f < 0){
        perror("File open failed.");
        return -1;
    }
    uint64_t number_of_inodes = sb_total_inode(device);
    off_t start_inode = lseek(device_f, 1024 + sizeof(struct lightfs_superblock) + number_of_inodes, SEEK_SET);
    if(start_inode == -1){
        perror("Failed to move to inode location.");
        close(device_f);
        return -1;
    }
    off_t inode = lseek(device_f, sizeof(struct lightfs_inode) * inode->inum, SEEK_CUR);
    if(inode == -1){
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