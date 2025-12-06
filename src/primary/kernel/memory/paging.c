#include "paging.h"
#include "memory.h"
#include "malloc.h"
#include <modules/modules.h>
#include <exception/exception.h>
#include <display/simple/display.h>

#define FLUSH_TLB(addr) __asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");
#define TOTAL_PAGE_ENTRIES (1024 * 1024)

extern void load_page_directory(u32 pd_addr);

void page_set_kernel(PageTableEntry* pt, u32 index) {
    pt[index].present = true;
    pt[index].rw = true;
    // We don't want any user access to our paging
    // directory so they can't see or modify it.
    pt[index].allowuser = false;
    // Remember that each increment of i represents 0x1000 bytes
    // so we can just directly assign it, and it would make an 
    // identity mapping.
    pt[index].address = index; 
}

PageDirectoryEntry* pd = (PageDirectoryEntry*)0x1000;

u16* page_directory_counts;

void paging_init() {
    // let's keep count of page tables to dynamically allocate and
    // free them as necessary.
    page_directory_counts = (u16*)kmalloc(sizeof(u16) * 1024);
    memset(page_directory_counts, 0, sizeof(u16) * 1024);

    // Ok so the smart thing to do here is
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
    pt[0].notforuse = 1;

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
        page_directory_counts[0]++;
    }

    // screw this this was a stupid idea to put
    // the page directory at 0x0000
    pt[0].present = false;

    // Our directory should be complete.
    // so we call the assembly helper.
    load_page_directory((u32)pd);
}

static void allocate_a_page_directory_entry(u32 pd_index) {
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

u32 alloc_page(u32 flags) {
    while (1) {
        u32 pd_index = rand() % 1024;
        u32 pt_index = rand() % 1024;

        if (!pd[pd_index].present) {
            allocate_a_page_directory_entry(pd_index);
        }

        PageTableEntry* pt = (PageTableEntry*)((u32)(pd[pd_index].table_addr << 12));
        PageTableEntry* entry = &pt[pt_index];
        if (entry->present || entry->notforuse) {
            continue;
        }

        u32 phys_addr = alloc_frame();
        entry->present = true;
        entry->address = phys_addr >> 12;
        entry->rw = true;
        entry->allowuser = true;
        if (flags & PAGEF_NOUSER) {
            entry->allowuser = false;
        }
        if (flags & PAGEF_READONLY) {
            entry->rw = false;
        }
        u32 virt_addr = (pd_index << 22) | (pt_index << 12);
        page_directory_counts[pd_index]++;
        FLUSH_TLB(virt_addr);
        return virt_addr;
    }
}

// We allocate a new 4KB page at the given virtual address
// mapping to the given physical address.
bool alloc_page_at_addr(u32 virt_addr, u32 flags) {
    // Get the directory index and table index
    u32 pd_index = (virt_addr >> 22) & 0x3FF;
    u32 pt_index = (virt_addr >> 12) & 0x3FF;

    // check if the page directory entry is present
    if (!pd[pd_index].present) {
        allocate_a_page_directory_entry(pd_index);
    }

    // get the page table
    PageTableEntry* pt = (PageTableEntry*)((u32)(pd[pd_index].table_addr << 12));
    PageTableEntry* entry = &pt[pt_index];

    if (entry->present || entry->notforuse) {
        // already allocated
        return false;
    }

    u32 phys_addr = alloc_frame();

    // set up the page table entry
    entry->present = true;
    entry->rw = true;
    entry->global = false;
    entry->allowuser = true; // allow user programs
    entry->address = phys_addr >> 12;
    if (flags & PAGEF_NOUSER) {
        entry->allowuser = false;
    }
    if (flags & PAGEF_READONLY) {
        entry->rw = false;
    }

    page_directory_counts[pd_index]++;

    FLUSH_TLB(virt_addr);
    return true;
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

    page_directory_counts[pd_index]--;

    FLUSH_TLB(virt_addr);

    // if the page table is now empty, free it
    if (page_directory_counts[pd_index] == 0) {
        kfree(pt);
        memset(&pd[pd_index], 0, sizeof(pd[pd_index]));
        FLUSH_TLB(virt_addr);
    }
}

// needs to generate a contiguous range of pages
u32 alloc_page_range(u32 pages, u32 flags) {
    if (pages == 0 || pages > TOTAL_PAGE_ENTRIES) {
        return 0;
    }

    u32 run_length = 0;
    u32 candidate_start = 0;
    bool found = false;

    for (u32 pd_index = 0; pd_index < 1024 && !found; pd_index++) {
        bool table_present = pd[pd_index].present;
        PageTableEntry* pt = table_present ? (PageTableEntry*)((u32)(pd[pd_index].table_addr << 12)) : null;

        for (u32 pt_index = 0; pt_index < 1024; pt_index++) {
            bool entry_free;
            if (table_present) {
                PageTableEntry* entry = &pt[pt_index];
                entry_free = (!entry->present) && (!entry->notforuse);
            } else {
                entry_free = true;
            }

            if (entry_free) {
                if (run_length == 0) {
                    candidate_start = (pd_index << 22) | (pt_index << 12);
                }
                run_length++;
                if (run_length == pages) {
                    found = true;
                    break;
                }
            } else {
                run_length = 0;
            }
        }
    }

    if (!found) {
        return 0;
    }

    for (u32 j = 0; j < pages; j++) {
        alloc_page_at_addr(candidate_start + (j * 0x1000), flags);
    }

    return candidate_start;
}

bool alloc_page_range_at_addr(u32 start_addr, u32 pages, u32 flags) {
    // check if the range is free
    for (u32 i = 0; i < pages; i++) {
        u32 addr = start_addr + (i * 0x1000);
        u32 pd_index = (addr >> 22) & 0x3FF;
        u32 pt_index = (addr >> 12) & 0x3FF;

        if (pd[pd_index].present) {
            PageTableEntry* pt = (PageTableEntry*)((u32)(pd[pd_index].table_addr << 12));
            if (pt[pt_index].present) {
                // already allocated
                return false;
            }
        }
    }

    // allocate the range
    for (u32 i = 0; i < pages; i++) {
        alloc_page_at_addr(start_addr + (i * 0x1000), flags);
    }
    return true;
}

void allow_null_page_read() {
    // for the sake of data that might only be stored
    // in the first 4kb of memory,
    // the first page table entry will be present
    // but read only including for kernel.
    PageTableEntry* pt = (PageTableEntry*)((u32)(pd[0].table_addr << 12));
    pt[0].present = true;
    pt[0].rw = false;
    pt[0].allowuser = false;
    pt[0].address = 0x0 >> 12;
    page_directory_counts[0]++;
    FLUSH_TLB(0x0);
}

void disallow_null_page() {
    // for the sake of data that might only be stored
    // in the first 4kb of memory,
    // the first page table entry will be not present
    PageTableEntry* pt = (PageTableEntry*)((u32)(pd[0].table_addr << 12));
    pt[0].present = false;
    page_directory_counts[0]--;
    FLUSH_TLB(0x0);
}