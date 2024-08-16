# TO-DO:
## inode operations
 - validate the offset calculation method

## file operations
 - Trunication

## address space operations
 - Writepage
    - sb_getblk() to get and write it in
 - begin operations
    - Should begin with first locking the file, and then readpage operation
 - end operations
    - Unlock the file