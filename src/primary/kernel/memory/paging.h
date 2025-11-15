#ifndef PAGING_H
#define PAGING_H

#include <modules/modules.h>

/*
Translation of a virtual address into a physical address 
first involves dividing the virtual address into three parts: 
the most significant 10 bits (bits 22-31) specify the index of
the page directory entry, the next 10 bits (bits 12-21) specify 
the index of the page table entry, and the least significant
12 bits (bits 0-11) specify the page offset.

For example, consider the virtual address 0x12345678.
In binary, this address is represented as:
0001 0010 0011 0100 0101 0110 0111 0111 1000
The breakdown is as follows:
- Page Directory Index (bits 22-31): 0001001000 (0x48)
- Page Table Index (bits 12-21): 1101000101 (0x345)
- Page Offset (bits 0-11): 011001111000 (0x678)

For 0xc0000000, the breakdown is:
- Page Directory Index (bits 22-31): 1100000000 (0x300)
- Page Table Index (bits 12-21): 0000000000 (0x000)
- Page Offset (bits 0-11): 000000000000 (0x000)
*/

typedef struct PageDirectoryEntry {
    // Is the page currently present at the moment in memory?
    // When implementing swapping, this bit will be set to 0 when the 
    // page is on disk. Accessing it will cause a page fault, which we
    // should intercept, handle, get the page from disk, and set to 1,
    // allowing the program to continue.
    u8 present        : 1;
    // Read-Write. WP bit in CR0 configures if this also applies to kernel
    u8 rw             : 1;
    // User/Supervisor. 0: only ring 0 can access. 1: ring 3 and ring 0.
    u8 allowuser      : 1;
    // PWT, controls Write-Through' abilities of the page. 
    // If the bit is set, write-through caching is enabled. 
    // If not, then write-back is enabled instead. - OSDEV
    // I think it just means there's assisted write caching
    // for performance. 1 to enable, 0 to disable.
    u8 write_through  : 1;
    // Cache Disable bit. This should most likely be read caching.
    // 1 to disable, 0 to enable.
    u8 cache_disable  : 1;
    // Accessed bit. Set by the CPU when the page is accessed.
    // We can set all accessed bits to zero, and check which pages
    // remain at zero. Those are the ones we would swap out first.
    u8 accessed       : 1;
    // Reserved for our own use if necessary
    u8 _reserved      : 1;
    // Page size: for our purposes this should always be 0 meaning 4kb pages
    u8 page_size      : 1;
    // More unused bits (8-11)
    u8 _reserved2     : 4;
    // The upper 20 bits of the physical address of the page table.
    // Of course should be aligned to a 4KB boundary.
    u32 table_addr    : 20;
} __attribute__((packed)) PageDirectoryEntry;

// Very similar to a PageDirectoryEntry so not too much description
typedef struct PageTableEntry {
    u8 present        : 1;
    u8 rw             : 1;
    u8 allowuser      : 1;
    u8 write_through  : 1;
    u8 cache_disable  : 1;
    // Accessed bit. Set by the CPU when the page is accessed.
    // Swap these out second to last.
    u8 accessed       : 1;
    // Dirty bit. Set by the CPU when the page is written to.
    // These pages should be swapped out last since they
    // are modified most recently.
    u8 dirty          : 1;
    // Controls the Page Attribute Table but we don't
    // really know what that is so its reserved.
    u8 pat            : 1;
    // 'Global' tells the processor not to invalidate 
    // the TLB entry corresponding to the page upon a MOV 
    // to CR3 instruction. Bit 7 (PGE) in CR4 must be set 
    // to enable global pages. - OSDEV
    // Basically if this page isn't likely to get deleted
    // and is important, then don't reload this page
    // set 1 to enable, 0 to disable.
    u8 global         : 1;
    // The next 3 bits are considered 'available'
    // This means we can use them 
    u8 _reserved      : 3;
    // The actual physical memory
    u32 address    : 20;
} __attribute__((packed)) PageTableEntry;

void paging_init();

void alloc_page(u32 virt_addr);
void free_page(u32 virt_addr);

#endif