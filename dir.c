#include "lightfs.h"
#include <unistd.h>


int lightfs_writedir(const struct d_entry *d_entry, int device, uint64_t inode){
    struct d_header *d_header;
    int ret = lightfs_read_dir_header(device, d_header, inode);
    if(ret > 0){
        perror("Failed to read directory header.");
        return -1;
    }
    off_t header_location = lseek(device, 1024 + sizeof(struct lightfs_superblock) + (sb_total_inode(device) / 8) + (sizeof(struct lightfs_inode) * sb_total_inode(device)) + (sb_total_block(device) / 8) + (sb_bs(device) * inode), SEEK_SET);
    if(header_location == -1){
        perror("Failed to move to directory header location.");
        return -1;
    }
    uint64_t current_item_num = d_header->total_item_num;
    d_header->total_item_num++;
    if (write(device, d_header, sizeof(struct d_header)) == -1) {
        perror("failed to write directory header.");
        return -1;
    }
    off_t start_dir = lseek(device, sizeof(struct d_header) + (sizeof(struct d_entry) * current_item_num), SEEK_CUR);
    if(start_dir == -1){
        perror("Failed to move to directory location.");
        return -1;
    }
    if (write(device, d_entry, sizeof(struct d_entry)) == -1) {
        perror("failed to write directory entry.");
        return -1;
    }
    return 0;
}