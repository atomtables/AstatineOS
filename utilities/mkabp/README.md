# MKABP: make astatine boot partition
creates a boot partition for AstatineOS using the specified 
specifications as specified in the specification sheet shown below.

## partition layout
### 0x0000 - 0x01ff: VBR (512 bytes)
- 0xD9 (partition identitier),
- "AstBoot" for signature
- etc. variables about size
### File list
for a file
```c
typedef struct FILE_ENTRY {
    u8   flags; // 0x00 (0)
    char name[8]; // 0x01-0x09 (1-9)
    char extension[3]; // 0x0A-0x0C (10-12)
    ptr  start_addr; // 0x0D-0x10 (13-16)
    u32  size; // 0x11-0x14 (17-20)
} __attribute__((packed)) FILE_ENTRY; // size 20 bytes
```
for a folder (unimplemented right now)
- size of an entry is also 20 bytes
- unlike a file, this encompasses numerous files and possibly more folders, so
  it will also require an amount of files/folders within it.
- flag will be set to denote folder
- size will be amount of files in folder
- **the bootloader iterates through 
each entry, finding each within 512 bytes, then 
loading the entire boot executable into memory 
which then sets up the rest of the utilities 
such as a very simple driver to call the kernel**
### Boot file
to successfully create a boot partition, you **must** have a boot file. (BOOT.AEX)
the file can be named anything, but it must exist and have its file name in the 
first sector
of the partition. this boot file should usually just be left alone but can also be
intercepted by a developer if necessary. it'll probably include verification checks
later.

files can be located anywhere on the partition but current implementation keeps everything sector aligned.