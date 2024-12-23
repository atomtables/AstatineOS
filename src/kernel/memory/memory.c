//
// Created by Adithiya Venkatakrishnan on 23/07/2024.
//

#include <modules/modules.h>
#include "memory.h"

#include <display/simple/display.h>
#include <exception/exception.h>

// Memory is between 0x100000 and 0x1fffff
// we will absolutely reduce this "heap" size later
// but for now this is enough

//

struct mem_bounds {
    u32 start;
    u32 end;
    u8  memfree[8192];
} mem;

void init_mem() {
    mem.start = MEM_BLOCK_START;
    mem.end = MEM_BLOCK_END;
}

/**
 * funnily enough all the code below was an APCSA collegeboard problem lmao
 */
int laughable = 1;

bool is_block_free(const int block) {
    // since the one bit is stored in a byte, we need to divide by 8
    const int byte = block / 8;
    const int bit = block % 8;
    return !BIT_GET(mem.memfree[byte], bit);
}

void free_block(const int block) {
    const int byte = block / 8;
    const int bit = block % 8;
    mem.memfree[byte] = BIT_SET(mem.memfree[byte], bit, 0);
}

void reserve_block(const int block) {
    const int byte = block / 8;
    const int bit = block % 8;
    mem.memfree[byte] = BIT_SET(mem.memfree[byte], bit, 1);
}

/// FUNCTION: malloc
/// very rudimentary/alpha, will be improved at some point in the future.
void* malloc(const int bytes) {
    int blocks;
    if (bytes % MEM_BLOCK_BYTE_SIZE == 0) {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE;
    } else {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE + 1;
    }

    // find the first block of memory that is free
    int block = 0; int other_block = 0; int length = 0;
    while (block < 8192) {
        if (is_block_free(block)) {
            length++;
            if (length == blocks) {
                // we have enough blocks to allocate, now we can reserve blocks
                for (int i = block; i < block + length; i++) {
                    reserve_block(i);
                }
                return (void*)(block * MEM_BLOCK_BYTE_SIZE + MEM_BLOCK_START);
            }
        } else {
            block = other_block;
            length = 0;
        }
        other_block++;
    }

    // at this point, we have no memory to allocate
    // TODO: handle heap full
    panic("no more mem");
    FATALERROR();
}

void* calloc(const int bytes) {
    void* ptr = malloc(bytes);
    memset(ptr, 0, bytes);
    return ptr;
}

void* realloc(void* addr, u32 oldsize, u32 size) {
    void* new_addr = malloc(size);
    memcpy(new_addr, addr, size);
    free(addr, oldsize);
    return new_addr;
}

/// FUNCTION: free
void free(void* ptr, const int bytes) {
    int blocks;
    if (bytes % MEM_BLOCK_BYTE_SIZE == 0) {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE;
    } else {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE + 1;
    }

    int block = ((u32)ptr - MEM_BLOCK_START) / MEM_BLOCK_BYTE_SIZE;
    for (int i = block; i < block + blocks; i++) {
        free_block(i);
    }
}

/// someone else do me a favor and implement a freemem function
