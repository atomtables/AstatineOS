//
// Created by Adithiya Venkatakrishnan on 23/07/2024.
//

#include "modules/modules.h"
#include "memory.h"

#include <display/display.h>

// Memory is between 0x100000 and 0x1fffff
// we will absolutely reduce this heap size later
// but for now this is enough

struct mem_bounds {
    u32 start;
    u32 end;
} mem_bounds;

void init_mem() {
    mem_bounds.start = MEM_BLOCK_START;
    mem_bounds.end = MEM_BLOCK_END;
}

/// FUNCTION: malloc
/// very rudimentary/alpha, will be improved at some point in the future.
void* malloc(const int bytes) {
    const int blocks = bytes / MEM_BLOCK_BYTE_SIZE + 1;
    void* ptr = (void*) mem_bounds.start;
    mem_bounds.start += blocks * MEM_BLOCK_BYTE_SIZE;

    return ptr;
}

/// someone else do me a favor and implement a freemem function