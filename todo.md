# TO-DO:
## inode operations
 - make file operations, dir operations, link operations
 - validate the offset calculation method
 - Retrieval of data block numbers
    - Inode structure will have one structure that points to the block containing data block numbers
 - New inode container structure
    - pointer to block number block
    - 
## file operations
 - think of a file offset calculation method when open()

## address space operations
 - Readpage
    - use get_block() to get certain block, and copy that to user space.
 - Writepage
    - sb_getblk() to get and write it in
 - begin operations
    - Should begin with first locking the file, and then readpage operation
 - end operations
    - Unlock the file
## bitmap
 - Think of a syncing operation