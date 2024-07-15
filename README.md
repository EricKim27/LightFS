# LightFS
This is my attempt to create a UNIX file system. It is still far from being completed and working.

The name is LightFS not because I made it very light, it's because I didn't considered that much and made this with a very light information about file systems.

I'm still learning about file systems, so it'll take some time to complete it.

[to do list](todo.md)

[Korean Documentation](https://erikkim27.notion.site/LightFS-0971ce0a06a648a8b92a4cdf36e7749d)

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

directory blocks(blocks containing dentry) contain a head structure which contains the total number of items(whether directory, file, or links) in the directory.
