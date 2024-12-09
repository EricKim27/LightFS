#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/ioctl.h>

struct lightfs_superblock {
    uint64_t inode_block_num;
    uint64_t total_block_count;
    uint64_t data_block_num;
    uint64_t free_inode;
    uint64_t free_data;
    uint64_t root_inode;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t mount_time;
    uint32_t last_check_time;
    uint32_t created_os;
    uint32_t write_time;
    uint16_t magicsig;
    uint16_t state;
    uint16_t error;
    void* inode_bmap;
    void* data_bitmap;
    char padding[928];
};
struct timespec64 {
    int64_t tv_sec;
    int64_t tv_nsec;
};
struct lightfs_inode {
    uint32_t i_mode;
    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_size;
    struct timespec64 i_atime;
    struct timespec64 i_mtime;
    struct timespec64 i_ctime;
    uint32_t blocks;
    uint32_t block_no_blk;
    char padding[184];
};
void print_lightfs_inode(struct lightfs_inode *ino) {
    printf("i_mode: %u\n", ino->i_mode);
    printf("i_uid: %u\n", ino->i_uid);
    printf("i_gid: %u\n", ino->i_gid);
    printf("i_size: %u\n", ino->i_size);
    printf("blocks: %u\n", ino->blocks);
    printf("block_no_blk: %u\n", ino->block_no_blk);
}
void print_lightfs_superblock(struct lightfs_superblock *sb) {
    printf("inode_block_num: %lu\n", sb->inode_block_num);
    printf("total_block_count: %lu\n", sb->total_block_count);
    printf("data_block_num: %lu\n", sb->data_block_num);
    printf("free_inode: %lu\n", sb->free_inode);
    printf("free_data: %lu\n", sb->free_data);
    printf("root_inode: %lu\n", sb->root_inode);
    printf("block_size: %u\n", sb->block_size);
    printf("inode_size: %u\n", sb->inode_size);
    printf("mount_time: %u\n", sb->mount_time);
    printf("last_check_time: %u\n", sb->last_check_time);
    printf("created_os: %u\n", sb->created_os);
    printf("write_time: %u\n", sb->write_time);
    printf("magicsig: %u\n", sb->magicsig);
    printf("state: %u\n", sb->state);
    printf("error: %u\n", sb->error);
}
void get_root_ino(int fd, struct lightfs_superblock *sb, struct lightfs_inode *ino){
    off_t ino_offset = 2048 + ((sb->data_block_num / 1024) + 1) * 1024 + ((sb->inode_block_num / 1024) + 1) * 1024 + sb->root_inode * sizeof(struct lightfs_inode);
    lseek(fd, ino_offset, SEEK_SET);
    if(read(fd, ino, sizeof(struct lightfs_inode)) < 0) {
        return;
    }

}
int main(int argc, char **argv) {
    if(argc < 2) {
        printf("usage: %s <rawimg>\n", argv[0]);
        return 1;
    }
    char* filename = argv[1];
    struct lightfs_inode *ino = (struct lightfs_inode *)malloc(sizeof(struct lightfs_inode));
    int fd;
    off_t pos;
    if((fd = open(filename, O_RDONLY)) < 0) {
        return 1;
    }
    struct lightfs_superblock *buf = (struct lightfs_superblock *)malloc(sizeof(struct lightfs_superblock));
    if((pos = lseek(fd, 1024, SEEK_SET)) < 0) {
        return 2;
    }
    if(read(fd, buf, sizeof(struct lightfs_superblock)) < 0) {
        return 3;
    }
    print_lightfs_superblock(buf);
    get_root_ino(fd, buf, ino);
    print_lightfs_inode(ino);
    free(buf);
    free(ino);
    close(fd);
    return 0;
}