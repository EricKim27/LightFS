# LightFS
This is my attempt to create a UNIX file system. It is still far from being completed and working.

The name is LightFS not because I made it very light, it's because I didn't considered that much and made this with a very light information about file systems.

considering about changing the name to 1kfs, because it operates on 1KiB logical blocks.

Most operations are in completion, but I'm still far away from making the mkfs code and further fixing problems within the driver.
[to do list](todo.md)

[Korean Documentation](korean.md)

Contributions to this code are welcome.

# Structure
| area of the filesystem |
| ---------- |
| 1024 bytes reserved |
| Superblock |
| inode bitmap |
| data bitmap |
| inode area |
| data block area |

The entire structures in the filesystem is designed to fit within the 1024 * a(a can be int or 1/2^n(n's range is 1~10)) bytes. The superblock is 1024 bytes,inode is 256 bytes, dentry is 64 bytes, and blocks are 1024 * n(preferrably 4KiB) bytes. The padding in the structure is for matching this size.

The calculated max file size of this system is 4MiB, and the max number of dentries one directory can have is 64(estimating that the block size is 4KiB).

directory blocks(blocks containing dentry) contain a head structure which contains the total number of items(whether directory, file, or links) in the directory.
