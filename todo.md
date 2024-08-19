# TO-DO:
## inode operations
 - validate the offset calculation method
   - all calculation methods need to be fixed to have the initial block & inode number as 1; not 0

## file operations
 - Trunication

## address space operations
 - Writepage
    - sb_getblk() to get and write it in
 - begin operations
    - Should begin with first locking the file, and then readpage operation
 - end operations
    - Unlock the file