# AstatineOS's syscall build

After a long think on my balcony, with a can of [Limca] in one hand and
a smoking [chimney] behind me, I proceeded to think about the syscall
API that AstatineOS has to uphold.

Okay, I'm lying about the balcony, the chimney, and the Limca. I haven't
opened the Limca yet, and I don't have a chimney. But we do need a different
way to express syscalls in AstatineOS, and unlike those geniuses at
Unix, we don't need any revolutionary "everything is a file" system.

I also want to implement an extremely verbose syscall system that isn't
allowing for type unsafe syscalls that handle the entire system at
once, because I don't like that and this is my project. Take that,
chimney.

This is all going to be an utter pain to rewrite once I (actually)
switch to 64-bit long mode (thank you, AMD, for sponsoring this
project in my dreams), but I would like to see DOOM ported to this
version of the project before I port it to 64-bit, just as a final 
send off and a milestone project. I don't know if I should even do a
32-bit compatibility layer, but I probably won't.

## exit codes are always signed 16-bit integers.

## program control: 0x0-0xf (0-15)
The first 16 syscalls will be for program controls. I'm not going
to implement these until we can get a good multitasking system going,
but it would be nice to get the structure of these ready within the
system.

### 0x0: `void abort(void);`
Ends the execution of the current program immediately, requiring
the kernel to perform clean-up of all requested endpoints (such as files,
devices, sockets, and pages). Because the program will not have any
runtime to react to an abort once it has been done, buffers to write
to files should be stored within kernel memory.

abort() will provide an exit code of -1 to any parent process that wishes
to collect information about it. Unlike unix, abort() will not send a
signal, but will dump the contents of the executable if the
executable would allow it to do so (debugging).

### 0x1-0xf: starting program, IPC like message passing, pipes, signals
will be implemented when scheduler is written.

## 0x10-0x1f: reading, writing, opening etc.
These are for controlling devices, files, etc. and having stuff to 
control both IO devices (that the kernel allows the user to access) 
and write to files as well.

### files, devices, and the "VFS"
Unlike Unix, this exquisite operating system will contain no mention of
"everything is a file" philosophy. Filesystems are mounted as a prefix
on the root ("/"). Example: the boot partition must always be mounted
as "/primary", and devices will have a default mount point that relates
to their device (/floppy0), their connection type (/ata1), or a custom
name/mount point given by the user (/ihatewindows). 

Devices have no mount points in the VFS, but you can memory map
files.

### 0x10: `int file_open(char* file_path, uint32_t mode);`
This opens a file on a mounted filesystem, and returns a positive
integer identifying the opened file. On failure, a negative return
code will be given denoting what the error is.

Files are, by default, not locked, meaning that the last program
to write to a file will be the one that wins. Programs can manually
lock files, but these file locks are temporary. Hangs shouldn't be a
huge problem, we can have a time limit on file locks or do it like
Windows where a program has to respond to a file within 30 seconds of
another program requesting access.

Modes allowed:
- at least one of the following 3:
  - F_NONE(`1<<0`): do not open the file, but keep a handle for other uses (I saw this in linux)
  - F_READ(`1<<1`): open the file for reading
  - F_WRITE(`1<<2`): open the file for writing
- F_CREATE(`1<<3`): create the file if it does not already exist
- F_EXPECT_DIRECTORY(`1<<4`): expect a directory, or throw an error. should not be used with `F_EXPECT_FILE`
- F_EXPECT_FILE(`1<<5`): expect a file, or throw an error. should not be used with `F_EXPECT_DIRECTORY`
- F_TRUNCATE(`1<<6`): truncates the existing file to zero. should be used in conjunction with `F_WRITE`.
- F_NO_CACHE(`1<<7`): removes any caching/buffers and writes to file directly. should be used in conjunction with `F_WRITE`.

Error codes:
- -1: invalid file path (file can not exist at this path)
- -2: invalid mode
- -3: lack of permissions (write when only read allowed)
- -4: missing file (if mode does not include F_CREATE)

### 0x11: `int device_open(enum DeviceType device, int id, uint32_t mode);`
This opens a device attached to the system. Unlike files, there is no
standard procedure on how to interact with a device, other than the
procedure clearly documented for the type of device that is being 
opened (which is dependent on the `device` passed).

In general, openable devices will have some sort of interface other than
open implemented, allowing them to do something useful. Modes in a
device_open also serve to verify that the selected device does, in fact,
provide the requested behaviour. Selecting no modes keeps the device busy,
but does not allow any action.

While file(systems) are required to implement all file operations, devices
are under no such expectation. Separating the opening of files from 
devices allows the expectations got from a device to be
different from what would be expected from a file: this is a fundamental
building block of AstatineOS.

Modes allowed:
- D_READ(`1<<0`): read data from the device (sends a read packet, and
                  allows the device to return whatever it wants)
- D_WRITE(`1<<1`): write data to the device (same thing)
- D_GETINFO(`1<<2`): get info about the device (block devices might
                     return data about size, bs, etc.)
- D_CONFIGURE(`1<<3`): configure the device using commands
                       specific to the device type
- D_MEMORYMAP(`1<<4`): allow memory mapping of the device (to access a certain command, for example)
                       (unlike files, this is managed by the device driver)

### 0x12: `ssize_t read(int handle, void* buffer, size_t count);`
The exact one from POSIX. Returns the amount of bytes read, or
a negative error code. Can be used for any handle, device or file.

### 0x13: `ssize_t write(int handle, const void* buffer, size_t count);`
Also, the exact one from POSIX. same idea but for writes. Will
throw an error if you didn't request permissions to write.

### 0x14: `ssize_t seek(int handle, off_t offset, uint32_t flags);`
Also, the exact one from POSIX. Man I'm on a copy roll today!

Flags allowed:
- SEEK_SET(`0`): set file offset to n bytes (must be positive)
- SEEK_CUR(`1`): add n bytes to the current offset
- SEEK_END(`2`): set file offset to end + n bytes (must be negative)

### 0x15: `void* map_file(int handle, void* address, size_t size, off_t offset, uint32_t flags);`
This maps an opened file (through the handle given) into memory,
allowing it to be accessed without constant reads and writes. This
may only be used to map files, not devices, and is not allowed to get
more pages that you might want.

`address` can be used to request that the file is kept at or near
the address specified, of course this is completely dependent on how
the operating system wishes to do things. If `address` is not specified,
then the OS will keep the mapped area at an accessible page to the
program.

`mapfile` will lock the byterange mapped to memory separately, unless
`MF_NO_LOCK` is specified. This means that mapfile does not require the
handle to stay active to work, since it maintains its own mapping handle
within the kernel (even if `MF_NO_LOCK` is specified).

However, if `MF_NO_LOCK` is specified, the system
will allow for changes to be made to the memory segment that, in
the case of conflicts, will crash the program with a failed memory
write.

`mapfile` is basically the same as mmap, but only for files, and a little
bit more secure?.

Flags allowed:
- Mode switches (at least one must be selected, otherwise an error will occur)
  - MF_SHARE_PAGE(`1<<0`): this mode will reflect changes made to the page into the file, and if any other processes requests mmap to the same portion, it will share the same hardware frame.
  - MF_SEPARATE(`1<<1`): this mode will simply copy the section into memory, and no change will be reflected into the file.
- Access switches (if none are selected, the mapping will be made but cannot have anything done on it.)
  - MF_READ(`1<<2`): pages may be read.
  - MF_WRITE(`1<<3`): pages may be written. 
  - MF_EXECUTE(`1<<4`): pages may be executed. this is exclusive with write (no page may have RWX, only RW or RX)
- MF_LARGE(`1<<5`): will allocate large pages instead
- MF_FORCE_ADDRESS(`1<<6`): will require either the exact placement of the address to be mapped, or for mapping to completely fail
- MF_NO_LOCK(`1<<7`): will not lock the byterange, at the risk of desynchronisation
- MF_AVOID_SWAP(`1<<8`): will require that this page will not be swapped unless explicitly asked for. If this fails, the mapping will fail altogether.

### 0x16: `void* map_device(int handle, void* address, size_t size, uint32_t map_type, uint32_t flags);`
This maps an open device (through the handle given) into memory. However,
this requires a map_type: a device-type specific item that determines 
what exactly you will be mapping. 

For example: if you want to map the framebuffer of the video card,
you would open device "fb" and request a map_type 0 (framebuffer).
But if I want to access general registers as memory-mapped IO instead,
I could request map_type 1 (memory-mapped IO), allowing me to access
the VGA registers or whatever. 

Also, unlike mapfile, mapdevice is completely controlled by the
driver. If you, for example, map "fb" but your size is only half of
the actual framebuffer, the driver can just reject your request to
memory map.

Uses the same flags as mapfile, with a couple of exceptions:
- MF_SEPARATE(`1<<1`) is an invalid flag.

### 0x17: `int get_page(void* address, size_t size, uint32_t flags);`
Maps an anonymous page. Replicates *nix's mmap with PAGE_ANON or wtv
- MF_SHARED(`1<<0`) is an invalid flag.

### 0x18: `int sync_page(void* address, size_t size, uint32_t flags);`
Syncs a file-backed page manually to disk. Usually file pages marked
with `MF_SHARE_PAGE` should automatically write to disk, but there is
no guarantee of when it will be. This syscall manually writes to
page, and has the advantage of writing MF_SEPARATE to disk as well.
Replicates *nix's msync.

Flags allowed:
- SP_SYNCHRONOUS(`0<<0`): stay at execution until file is written to
- SP_ASYNCHRONOUS(`1<<0`): finish and write to file in background

### 0x19: `int change_page_flags(void* mapped_mem, uint32_t flags);`
Allows you to change the flags associated with a page. Replicates
*nix's mprotect
- Creation flags like `MF_LARGE` and related may not be changed

### 0x1a: `int unmap_pages(void* address, size_t size);`
Unmaps the pages associated. Default behaviour is as follows:
- File backed pages marked as `MF_SHARE_PAGE` will be saved immediately.
- File backed pages marked as `MF_SEPARATE` will be discarded.
- Any locks associated will be dropped on the memory and file/device.
- This function will cause the program to fault if the address returned was not given via a map function.
- If `size` is lower than the `size` that the original map was, only the amount declared by size will be unallocated, 
  the remaining must be unmapped through another call.

### 0x1b: `int remap_pages(void* address, size_t new_size, ...);`
I have to work out the implementation details of remap_pages.

### 0x1c: `int lock_file(int handle, off_t start, off_t end, uint32_t flags);`
Locks the file mentioned in the handle from being written to. This lock
is mandatory, and access to the file, depending on the flags passed,
will be denied accordingly. Locks can only be issued if the file
has no open handles, and a locked file can only be accessed once the
lock is lifted. In both cases, a poll syscall can be used to wait for 
either exclusive access to the file, or for the file to be accessed.

If it is determined that a file has been hogged by a program, it is
possible for its lock to expire. Such conditions will be addressed
when I am smart enough to know all about expiring locks and notifying
programs when a lock expires.

Flags allowed:
- LF_READ(`1<<0`): Lock other programs from reading a file
- LF_WRITE(`1<<1`): Lock other programs from performing a write.

### 0x1d: `int device_configure_register(int handle, int direction, int reg_id, int value);`
This allows a device to have a register be configured. For example, VGA
registers on graphics cards (which are often standardised), can all be
configured via this one-step solution. Keep in mind it's completely up
to the driver to handle register configuration, some drivers may not
even support it.

### 0x1e: `int device_configure_memory(int handle, int direction, int mem_id, void* buffer, size_t size);`
This allows a device to be configured through some sort of memory
packet. For example, if you want the VESA configuration packet from a
graphics card, you can get that using this syscall. You will find
advantage using this when specifically configuring data, since `write`
cannot be multiplexed (map_device can though).

### 0x1f: `int close_handle(int handle)`
This closes the handle and all associated resources. File handles with
data stored in buffers will be flushed back to storage. Locks on the
item will be cleared. File-backed memory maps will not be cleared, and 
locks maintained on memory ranges will still be active.

### 0x20: `int file_info(int handle, struct file_info* out, int flags);`
This allows you to get information about a file from its handle. Unlike
Unix, this is only for files, and will provide you with actually 
useful information about a file node.

The struct file_info is a struct that's pretty basic in nature: you
get file/folder, permissions for the current authenticated user
(based on the user holding the calling program, more later), the
size of the file, and last accessed/modified/etc. 

```c++
struct file_info {
    uint8_t is_file: 1;
    uint8_t allow_read: 1;
    uint8_t allow_write: 1;
    uint8_t allow_execute: 1;
    uint8_t reserved: 0;
    // Whether this will use Unix-like IDs or Windows like strings, this will be changed.
    uint32_t owner;
    size_t bytecount;
    time_t last_accessed;
    time_t last_modified;
};
```

Flags allowed:
- FI_SYMLINK(`1<<0`): get info about the symlink, not about the underlying file.

### 0x21: `int device_info(int handle, struct device_info* out, int flags);`
Same idea as the other one, but uses a different struct that involves
information about the device. This function call is guaranteed to succeed
for every device, but may not have full population of information
because some devices may not provide all information necessary.

```c++
struct device_info {
    uint8_t allow_mmap: 1;
    uint8_t allow_read: 1;
    uint8_t allow_write: 1;
    uint8_t allow_configure: 1;
    uint8_t reserved: 0;
    // The owner of every device in Astatine is the kernel, so 
    // device access is totally up to whether the kernel thinks
    // the user should get it (usually a matter of permissions)
    uint16_t vendor_id;
    uint16_t product_id;
    uint32_t revision_id;
    
    // basically the type you used before
    uint8_t device_class;
    // more useful info
    uint8_t bus_type;
    
    // serial number for identification
    char serial_number[64];
};
```