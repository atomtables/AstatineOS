# astatine memory map

system stores this data as so

- `0x0`-`0x1000`: null page, reserved for bios reads
- `0x1000`-`0x2000`: 
  - `boot.aex`: reserved for `struct astatine_configuration`
  - `kernel`: page directory table
- `0x2000`-`0x3000`: 
  - `boot.aex`: reserved for e820 memory map
  - `kernel`: rewritten to be the kernel page table
- `0x10000`-`0x9ffff`: reserved for kernel executable memory
- `0xa0000`-`0xfffff`: x86 memory hole
- `0x100000`-`0x3fffff`: reserved for kernel data

other pages are dynamically allocated when the system is online, 
and can include kernel pages or user pages. kernel pages are allocated
at the smallest possible virtaddr they can be.