# NeoFS

A simple implementation of a filesystem

## What is NeoFS

NeoFS began as a filesystem made just because I (the developer) just could not take my time to read the documentation of the FAT or ext filesystem to understand it. Instead in a more interesting way of learning how filesystems work NeoFS was created. The filesystem has then been implemented in my own micro kernel.

Currently NeoFS supports: Dirs, read/write to files, removal of files. All of this can be tested with the tools compiled with the makefile.

Now NeoFS is still actively being developed to be more usable as a filesystem.

## Testing NeoFS

Testing NeoFS is currently only possible on .img files on the computer and not on a real harddrive. On a .img file NeoFS can easily be installed using the `mkfs.neofs` command and then passing it the disk option `--disk /path/to/disk.img`. Then you can play around with changing the content on disk with the rest of the commands: `copy.neofs`, `read.neofs`, `remove.neofs`.

All commands are easily compiled with the provided Makefile.

### Commands explaination

--verbose, -v can be applied to all commands to show a more detailed logging.

- **mkfs.neofs**: This creates a NeoFS filesystem on a file.img file passed by the `--disk /path/to/file` option. The command also provides a optional `--block_size` which can change the block size from the standard 1024 bytes to any provided number.

- **copy.neofs**: This copies the file provided with the argument `if=/path/to/file` to the path on disk privided with `trgt=/path/on/neofs_disk`. If the file should have any flags set you use `--flags` followed by the flag(s) like `r`, `w` or `x`. Like other commands this also needs the `--disk` to know what disk to copy to.

- **read.neofs**: This takes the first argument which isn't tied to any option starting with a `--` and treats it as the path to read on the disk. Like other commands this also needs the `--disk` to know what disk to read from.

- **remove.neofs**: This uses the same argument parsing as read but deletes the file on disk instead of reading it.