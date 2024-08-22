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
#include <linux/fs.h>

#define FILL_FS_BYTES 0x22
#define LOGICAL_SIZE 1024
char bytes = FILL_FS_BYTES;

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

struct lightfs_dentry {
    char filename[60];
    uint64_t inode;
};
struct lightfs_d_head {
    uint64_t magic;
    uint64_t item_num;
    char padding[48];
};

void get_time(struct timespec64 *ts)
{
    struct timespec ts_now;
    clock_gettime(CLOCK_REALTIME, &ts_now);
    ts->tv_sec = ts_now.tv_sec;
    ts->tv_nsec = ts_now.tv_nsec;
}
size_t calculate_inode_block_numbers(long long size, int block_size, int inode_size) {
    long long usable_size = size - 2048; // Subtract overhead: 2048 bytes reserved
    size_t a;
    int found = 0;

    // Iterate over possible values of a
    for (a = 0; a <= 100000000; a++) { // Assume an upper limit for a
        // Calculate the values of the ceiling functions
        size_t x = (size_t)ceil(a / LOGICAL_SIZE)*LOGICAL_SIZE;  // Inode bitmap blocks
        size_t y = (size_t)ceil((a *inode_size) / LOGICAL_SIZE);  // Inode area blocks
        
        // Calculate the size based on the current value of a
        long long calculated_size = 2048 + 2 * x + 1024 * y + block_size * a;
        if (calculated_size >= usable_size) {
            found = 1;
            break;
        }
    }

    // Return the found value or -1 if not found
    if (found) {
        return a-1;
    } else {
        return -1; // Indicate that no valid a was found
    }
}
int fill_disk(int fd) 
{
    struct stat st;
    if(fstat(fd, &st) == -1){
        perror("Failed to get fstat: ");
        return -1;
    }

    int *bytes_to_write = (int *)&bytes;
    size_t i;
    for(i=0; i<st.st_size; i++){
        if(write(fd, bytes_to_write, sizeof(bytes)) < 0) {
            perror("write error!: ");
            return -2;
        }
    }
    return 0;
}

struct lightfs_superblock *fill_super(size_t ib_num, size_t fs_size)
{
    struct lightfs_superblock *sb = (struct lightfs_superblock *)malloc(sizeof(struct lightfs_superblock));
    memset(sb, 0, sizeof(struct lightfs_superblock));
    srand(time(NULL));
    sb->block_size = 4096;
    sb->inode_size = 256;
    sb->data_block_num = ib_num;
    sb->inode_block_num = ib_num;
    sb->free_inode = ib_num;
    sb->root_inode = ib_num + 1;
    while(sb->root_inode > ib_num){
        sb->root_inode = (uint64_t)(rand() % (ib_num + 1 - 0) + 0);
    }
    sb->free_data = ib_num;
    sb->state = 0;
    sb->error = 0;
    sb->magicsig = 0x20070207;
    
    return sb;
}
struct lightfs_inode *set_root_inode(uint32_t max_inode)
{
    struct lightfs_inode *inode = (struct lightfs_inode *)malloc(sizeof(struct lightfs_inode));
    memset(inode, 0, sizeof(struct lightfs_inode));
    srand(time(NULL));
    inode->i_mode = 0766;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->blocks = 1;
    inode->block_no_blk = max_inode + 1;
    while(inode->block_no_blk > max_inode){
        inode->block_no_blk = (uint32_t)(rand() % (max_inode + 1 - 0) + 0);
    }
    get_time(&inode->i_atime);
    inode->i_ctime = inode->i_atime;
    inode->i_mtime = inode->i_atime;

    return inode;
}
int write_ibitmap(int fd, uint32_t ino)
{
    size_t offset = 2048 + ino - 1;
    uint8_t val = 1;
    lseek(fd, offset, SEEK_SET);
    write(fd, &val, sizeof(val));
    return 0;
}
int write_bbitmap(int fd, uint32_t bnum, struct lightfs_superblock *sb)
{
    size_t offset = 2048 + ((sb->data_block_num / LOGICAL_SIZE+1) * LOGICAL_SIZE) + bnum - 1;
    lseek(fd, offset, SEEK_SET);
    uint8_t val = 1;
    write(fd, &val, sizeof(val));
    return 0;
}
int fill_zeroes(int fd)
{
    char *zero_buffer = calloc(1, 4096);
    ssize_t written;
    while((written = write(fd, zero_buffer, 4096)) > 0) {

    }
    if(written < 0){
        perror("Error while writing:");
        return -EXIT_FAILURE;
    }
    return written;
}
char *init_block_no_block(uint32_t bnb, int max_block)
{
    char *block = (char *)malloc(4096);
    memset(block, 0, 4096);
    if(block == NULL){
        perror("Failed during init_block_no_block:");
        return NULL;
    }
    srand(time(NULL));
     int attempts = 0;
    uint32_t init_blk = max_block+1;
    while(init_blk > max_block) {
        init_blk = (uint32_t)(rand() % (max_block + 1 - 0) + 0);
        attempts++;
    }
    if(init_blk > max_block){
        perror("Failed to generate valid block number:");
        free(block);
        return NULL;
    }
    memcpy(block, &init_blk, sizeof(uint32_t));
    return block;
}
int init_dir(struct lightfs_superblock *sb, int fd, uint32_t block_num, uint32_t ino)
{
    char *block = (char *)malloc(4096);
    memset(block, 0, sizeof(struct lightfs_dentry));
    struct lightfs_d_head *head = (struct lightfs_d_head *)malloc(sizeof(struct lightfs_d_head));
    struct lightfs_dentry *dentry;

    //initalize dir block head
    head->item_num = 2;
    head->magic = 27;
    memcpy(block, head, sizeof(struct lightfs_d_head));
    free(head);

    dentry = (struct lightfs_dentry *)malloc(sizeof(struct lightfs_dentry));
    memset(dentry, 0, sizeof(struct lightfs_dentry));
    strcpy(dentry->filename,".");
    dentry->inode = ino;
    memcpy(block + sizeof(struct lightfs_d_head), dentry, sizeof(struct lightfs_dentry));
    free(dentry);

    dentry = (struct lightfs_dentry *)malloc(sizeof(struct lightfs_dentry));
    memset(dentry, 0, sizeof(struct lightfs_dentry));
    dentry->inode = ino;
    strcpy(dentry->filename,"..");
    free(dentry);

    size_t block_offset;
    if(sb->inode_block_num * 256 % 1024 == 0)
        block_offset = 2048 + (ceil(sb->data_block_num / LOGICAL_SIZE) * LOGICAL_SIZE * 2) + (sb->inode_block_num * 256) + (block_num - 1) * sb->block_size;
    else
        block_offset = 2048 + (ceil(sb->data_block_num / LOGICAL_SIZE) * LOGICAL_SIZE * 2) + (ceil(sb->inode_block_num * 256 / 1024)*1024) + (block_num - 1) * sb->block_size;
    lseek(fd, block_offset, SEEK_SET);
    write(fd, block, 4096);
    write_bbitmap(fd, block_num, sb);
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        printf("Usage: %s <device file>\n", argv[0]);
        return -EXIT_FAILURE;
    }
    printf("LightFS formatter v0.1\n");
    printf("Note: resetting the filesystem has errors, so you need to use dd to fill the disk with zeroes.\n");
    printf("Making filesystem to %s\n", argv[1]);
    int fd;
    struct lightfs_superblock *sb;
    struct lightfs_inode *inode;

    fd = open(argv[1], O_WRONLY);
    struct stat stat_buf;
    int ret = fstat(fd, &stat_buf);
    if(ret){
        perror("fstat error:");
        return -EXIT_FAILURE;
    }
    long int blk_size;
    if ((stat_buf.st_mode & S_IFMT) == S_IFBLK) {
        blk_size = 0;
        ret = ioctl(fd, BLKGETSIZE64, &blk_size);
        if (ret != 0) {
            perror("BLKGETSIZE64 error:");
            close(fd);
            return -EXIT_FAILURE;
        }
        stat_buf.st_size = blk_size;
    }

    size_t block_number = calculate_inode_block_numbers((long long)stat_buf.st_size, 4096, 256);
    if(block_number < 0){
        perror("Calc failed:");
        close(fd);
        return -EXIT_FAILURE;
    }
    printf("The inode number and block number is: %zu\n", block_number);
    //TODO: this causes endless loop
    /*
    if(fill_zeroes(fd) < 0){
        close(fd);
        return -EXIT_FAILURE;
    }
    */
    printf("Writing SuperBlock....");
    sb = fill_super(block_number, stat_buf.st_size);
    lseek(fd, 1024, SEEK_SET);
    write(fd, sb, sizeof(struct lightfs_superblock));
    printf("Done.\n");
    uint64_t ino = sb->root_inode;
	
    printf("Writing inode....");
    inode = set_root_inode(sb->inode_block_num);
    size_t inode_offset = 2048 + ((sb->data_block_num / LOGICAL_SIZE + 1) * LOGICAL_SIZE * 2) + (ino * 256);
    lseek(fd, inode_offset, SEEK_SET);
    write(fd, inode, sizeof(struct lightfs_inode));
    write_ibitmap(fd, sb->root_inode);
    printf("Done.\n");
    printf("Writing Block number Block to disk....");
    char *bnb = init_block_no_block(inode->block_no_blk, sb->inode_block_num);
    if(bnb == NULL){
        close(fd);
        free(sb);
        free(inode);
        free(bnb);
        return EXIT_FAILURE;
    }
    uint32_t root_block = *((uint32_t *)bnb);
    size_t bnb_offset;
    if(sb->inode_block_num * 256 % 1024 == 0)
        bnb_offset = 2048 + (ceil(sb->data_block_num / LOGICAL_SIZE) * LOGICAL_SIZE * 2) + (sb->inode_block_num * 256) + inode->block_no_blk * sb->block_size;
    else
        bnb_offset = 2048 + (ceil(sb->data_block_num / LOGICAL_SIZE) * LOGICAL_SIZE * 2) + (ceil(sb->inode_block_num * 256 / 1024)*1024) + (inode->block_no_blk - 1) * sb->block_size;
    
    lseek(fd, bnb_offset, SEEK_SET);
    write(fd, bnb, 4096);
    printf("Done.\n");
    if(init_dir(sb, fd, root_block, sb->root_inode) != 0){
        perror("Failed to initialize root block:");
        free(bnb);
        free(sb);
        free(inode);
        return -EXIT_FAILURE;
    }

    printf("filesystem created.\n");

    free(bnb);
    free(sb);
    free(inode);
    close(fd);
    return 0;
}
