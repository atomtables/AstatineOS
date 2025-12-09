#include "vfs.h"
#include <modules/modules.h>

// this is the interface that
// a filesystem driver must implement
typedef struct Filesystem {
    // The disk that this filesystem is owning
    struct Partition* partition;
    // This inspects the disk to make sure the partition is in fact
    // of the correct type for this filesystem.
    // @returns zero-indexed partition number on success, negative on failure
    int (*exists)   (struct Disk* disk);
    // Mounts the filesystem on the specified partition,
    // effectively taking ownership of that partition and locking it
    // from usage by other filesystems or raw disk access.
    int (*mount)    (struct Partition* partition, const char* prefix);
    int (*unmount)  (struct Partition* partition);
    // We need our file mechanisms
    // a file will be associated with an fd struct, which will
    // link to these functions for individual operations.
    int (*open)     (const char* path, struct fd* fd, u8 mode);
    int (*close)    (struct fd* fd);
    int (*read)     (struct fd* fd, void* buffer, u32 size);
    int (*write)    (struct fd* fd, const void* buffer, u32 size);
    int (*seek)     (struct fd* fd, i32 offset, u8 whence);
    int (*stat)     (const char* path, struct stat* statbuf);
    int (*unlink)   (const char* path);
    // now we need the directory operations
    // these are stored in fops on an fd as well, but
    // obviously have different behaviour.
    // diropen can also create the directory but not by default.
    int (*diropen)  (const char* path, struct fd* fd, u8 mode);
    // this just closes the directory fd
    int (*dirclose) (struct fd* fd);
    // this reads a directory entry (defined later)
    // basically, the underlying methods from libc will keep
    // track of position etc. so the syscall can just read the entry.
    int (*dirread)  (struct fd* fd, struct dirent* direntbuf);
} Filesystem;