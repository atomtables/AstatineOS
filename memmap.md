# astatine memory map

system stores this data as so

- `0x0`-`0x1000`: null page, reserved for bios reads
  - `0x500-0xfff`: reserved for boot.aex (IMPORTANT: boot.aex is currently approaching it's maximum size, since 0x1000 is where it stores `struct astatine_configuration`)
- `0x1000`-`0x2000`: 
  - `boot.aex`: reserved for `struct astatine_configuration`
  - `kernel`: page directory table
- `0x2000`-`0x3000`: 
  - `boot.aex`: reserved for e820 memory map
  - `kernel`: rewritten to be the kernel page table
- `0x8000`-`0xffff`:
  - reserved for `loader.aex`, the C program that loads the kernel into memory. currently 12 KB (0x8000-0xAD06) (IMPORTANT: make sure it doesn't exceed its size limit)
- `0x10000`-`0x9ffff`: reserved for kernel executable memory
- `0xa0000`-`0xfffff`: x86 memory hole
- `0x100000`-`0x3fffff`: reserved for kernel data

other pages are dynamically allocated when the system is online, 
and can include kernel pages or user pages. kernel pages are allocated
at the smallest possible virtaddr they can be.

initial bootloader ran by bios at `0x7c00`, VBR on the ABP partition
is also loaded at `0x7c00`, then loads boot.aex at `0x500`.