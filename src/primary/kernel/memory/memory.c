#include "memory.h"
#include "malloc.h"
#include "paging.h"
#include <modules/modules.h>
#include <display/simple/display.h>
#include <exception/exception.h>
#include <timer/PIT.h>

// Similar to how Linux does it,
// we can keep track of free memory pages
// based on memory maps.

// These are the datas provided by the BIOS
// Notable exclusion is the 0xA0000-0xFFFFF region
// which is usually reserved for MMIO like VGA, ACPI, etc.
// Only bytes marked specifically as usable RAM should be used.
typedef struct SMAP_entry {
	u32 BaseL; // base address uint64_t
	u32 BaseH;
	u32 LengthL; // length uint64_t
	u32 LengthH;
	u32 Type; // entry Type
	u32 ACPI; // extended
}__attribute__((packed)) SMAP_entry;

SMAP_entry* smap = (SMAP_entry*)0x2004;
u32 smap_count;

// simple bump allocator for frames
u32 next_frame_addr = 0;

u32 look_for_usable_frame(u32 after) {
    for (u32 i = 0; i < smap_count; i++) {
        SMAP_entry* entry = &smap[i];
        if (entry->Type == 1) { // usable RAM
            u64 base = ((u64)entry->BaseH << 32) | entry->BaseL;
            u64 length = ((u64)entry->LengthH << 32) | entry->LengthL;

            // align base to next 4KB boundary
            u64 aligned_base = (base + 0xFFF) & ~0xFFF;
            // align length to 4KB boundary
            u64 aligned_length = length - (aligned_base - base);
            aligned_length &= ~0xFFF;

            for (u64 addr = aligned_base; addr < aligned_base + aligned_length; addr += 0x1000) {
                if (addr >= after) {
                    return addr;
                }
            }
        }
    }
    return 0; // out of memory
}

void init_frame_alloc() {
    // let's look for the first memory address after 0x400000 that's ok to use
    next_frame_addr = 0x400000;//look_for_usable_frame(0x400000);
    if (next_frame_addr == 0) {
        // while(1);
        panic("Out of memory while allocating frame");
    }
}

u32 alloc_frame() {
    u32 frame = next_frame_addr;
    next_frame_addr = look_for_usable_frame(next_frame_addr + 0x1000);
    if (next_frame_addr == 0) {
        // while(1);
        panic("Out of memory while allocating frame");
    }
    return frame;
}

void free_frame() {}

void init_mem() {
    // the BIOS should have dumped the memory map at 0x2000
    // so let's allocate memory so it's not going to get overwritten.
    smap_count = *(u32*)0x2000;
    SMAP_entry* smap_copy = kmalloc(sizeof(SMAP_entry) * smap_count);
    memcpy(smap_copy, smap, sizeof(SMAP_entry) * smap_count);
    smap = smap_copy;

    u32 total_mem = 0;
    for (u32 i = 0; i < smap_count; i++) {
        SMAP_entry* entry = &smap[i];
        if (entry->Type == 1) { // usable RAM
            u64 base = ((u64)entry->BaseH << 32) | entry->BaseL;
            u64 length = ((u64)entry->LengthH << 32) | entry->LengthL;
            total_mem += (u32)length;
            printf("Usable RAM: Base 0x%x Length 0x%x\n", (u32)base, (u32)length);
        }
    }
    printf("Total Usable RAM: %d MiB\n", total_mem / 1024 / 1024);

    // set up paging first obv.
    paging_init();

    // we have free pages, but to allocate them, we need kmalloc
    // to have open space in the dymem region. This region is from
    // 0x100000 to 0x3FFFFF which is about 3MB. 
    // Using my current crappy implementation of kmalloc, it should be
    // autosetup.
    // kmalloc_init((void*)MEM_BLOCK_START, MEM_BLOCK_END - MEM_BLOCK_START);

    // Final thing we need is a frame address allocator
    // that can give out 4KB aligned physical addresses
    // for paging purposes.
    // We use the SMAP to avoid reserved regions.
    init_frame_alloc();
}
