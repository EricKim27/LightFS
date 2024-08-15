# TO-DO:
## inode operations
 - validate the offset calculation method
 - New inode container structure
    - pointer to block number block
    - 
## file operations
 - Trunication

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