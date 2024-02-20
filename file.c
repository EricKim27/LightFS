#include "lightfs.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int lightfs_fread(int fd, const char *path[]){
    uint64_t inode = sb_total_inode(fd);
    int goroot = lseek(fd, 1024 + sizeof(struct lightfs_superblock) + (inode * 8) + (inode * sizeof(struct lightfs_inode)) + sb_total_block(fd) + (2 * sb_bs(fd)) ,SEEK_SET);
    if(goroot == -1){
        return -1;
    }
    struct lightfs_inode *in;
    int ret = lightfs_get_inode(fd, path, &in);
    if(ret == -1){
        return -1;
    }
    if(in->uid != getuid()){
        if(in->gid != getgid()){
            return -1;
        }
        if(in->imode[1] <= 3){
            return -1;
        } 
    }
    if(in->imode[0] <= 3){
        return -1;
    }
}
