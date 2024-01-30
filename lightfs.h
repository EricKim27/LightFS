#include <stdint.h>

struct superblock {
    uint64_t number_of_inodes;
    uint64_t number_of_data;
};