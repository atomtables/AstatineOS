#include "paging.h"
#include "memory.h"
#include "malloc.h"
#include <modules/modules.h>
#include <exception/exception.h>
#include <display/simple/display.h>

#define FLUSH_TLB(addr) __asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");

extern void load_page_directory(u32 pd_addr);

void page_set_kernel(PageTableEntry* pt, u32 index) {
    pt[index].present = true;
    pt[index].rw = true;
    // We don't want any user access to our paging
    // directory so they can't see or modify it.
    pt[index].allowuser = true;
    // Remember that each increment of i represents 0x1000 bytes
    // so we can just directly assign it, and it would make an 
    // identity mapping.
    pt[index].address = index; 
}

PageDirectoryEntry* pd = (PageDirectoryEntry*)0x1000;

void paging_init() {
    CLI();
    // Ok so the smart (dumb) thing to do here is
    // put our paging directory at 0x1000.
    // Since we aren't going to use real-mode 
    // anymore.
    memset(&pd[0], 0, 4096);

    // The first 4 megabytes are identity-mapped.
    // We should not allow the user to access the table,
    // thank you very much.
    pd[0].present = true;
    pd[0].rw = true;
    pd[0].allowuser = false;
    pd[0].table_addr = 0x2000 >> 12; // page table at 0x1000
    pd[0].page_size = 0;

    // Our table address is at 0x1000, so we can set that up now.
    PageTableEntry* pt = (PageTableEntry*)(0x2000);
    memset(pt, 0, 4096);

    // page_set_kernel(pt, 0);      // 0x00000000 - 0x00000FFF
    page_set_kernel(pt, 1);      // 0x00001000 - 0x00001FFF

    // we also have to load our kernel address in page memory
    // based on how big it is. It's about 70k so just map starting
    // from 0x10000.
    // so this just makes it so the first 4MB of ram is identity-mapped and
    // ring 0 access only, since in this 4MB is where our kernel + dymem resides.
    for (u32 i = 0x0000; i < 0x400000; i += 0x1000) {
        u32 index = i >> 12;
        page_set_kernel(pt, index);
    }

    // screw this this was a stupid idea to put
    // the page directory at 0x0000
    pt[0].present = false;

    // Our directory should be complete.
    // so we call the assembly helper.
    load_page_directory((u32)pd);

    STI();
}

// We allocate a new 4KB page at the given virtual address
// mapping to the given physical address.
void alloc_page(u32 virt_addr) {
    // while(1);
    u32 phys_addr = alloc_frame();

    // Get the directory index and table index
    u32 pd_index = (virt_addr >> 22) & 0x3FF;
    u32 pt_index = (virt_addr >> 12) & 0x3FF;

    // check if the page directory entry is present
    if (!pd[pd_index].present) {
        // allocate a new page table
        PageTableEntry* new_pt = (PageTableEntry*)kmalloc_aligned(4096, 4096);
        // *(int*)new_pt = 0;
        memset(new_pt, 0, sizeof(PageTableEntry) * 1024);

        // set up the page directory entry
        pd[pd_index].present = true;
        pd[pd_index].rw = true;
        pd[pd_index].allowuser = true; // allow user programs
        pd[pd_index].table_addr = ((u32)new_pt) >> 12;
        pd[pd_index].page_size = 0;
    }

    // get the page table
    PageTableEntry* pt = (PageTableEntry*)((u32)(pd[pd_index].table_addr << 12));

    // set up the page table entry
    pt[pt_index].present = true;
    pt[pt_index].rw = true;
    pt[pt_index].global = false;
    pt[pt_index].allowuser = true; // allow user programs
    pt[pt_index].address = phys_addr >> 12;

    FLUSH_TLB(virt_addr);
}
void free_page(u32 virt_addr) {
    // Get the directory index and table index
    u32 pd_index = (virt_addr >> 22) & 0x3FF;
    u32 pt_index = (virt_addr >> 12) & 0x3FF;

    // check if the page directory entry is present
    if (!pd[pd_index].present) {
        return; // nothing to free
    }

    // get the page table
    PageTableEntry* pt = (PageTableEntry*)((u32)(pd[pd_index].table_addr << 12));
    if (!pt[pt_index].present) {
        return; // nothing to free
    }

    // clear the page table entry
    memset(&pt[pt_index], 0, sizeof(PageTableEntry));

    FLUSH_TLB(virt_addr);

    bool empty = true;
    // check from both ends towards the middle
    for (u32 i = 0; i < 512; i++) {
        if (pt[i].present || pt[1023 - i].present) {
            empty = false;
            break;
        }
    }

    if (empty) {
        kfree(pt);
        memset(&pd[pd_index], 0, sizeof(pd[pd_index]));
        FLUSH_TLB(virt_addr);
    }
}
