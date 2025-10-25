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
- size of an entry is 16 bytes
- 0x00: flags (for future use)
- 0x01-0x08: file name string (max of 8 characters with early termination)
- 0x09-0x0B: which sector the file is stored in (max offset of 2^24 sectors)
- 0x0C-0x0F: size of the file in bytes

for a folder
- size of an entry is also 16 bytes
- unlike a file, this encompasses numerous files and possibly more folders, so
  it will also require an amount of files/folders within it.
- 0x00-0x07: folder name string (max of 8 characters with early termination)
- 0x08-0x0F: number of files/folders within it (a max of 2^64 entries (unnecessary))
- **the bootloader iterates through 
each entry, finding each within 512 bytes, then 
loading the entire boot executable into memory 
which then sets up the rest of the utilities 
such as a very simple driver to call the kernel**
### Boot file
to successfully create a boot partition, you **must** have a boot file.
the file can be named anything, but it must exist and have its file name in the 
first sector
of the partition. this boot file should usually just be left alone but can also be
intercepted by a developer if necessary. it'll probably include verification checks
later.

files can be located anywhere on the partition, given they have 